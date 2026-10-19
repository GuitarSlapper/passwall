#include "CryptoService.h"
#include <QJsonDocument>
#include <QJsonParseError>
#include <sodium.h>
#include <stdexcept>

namespace passwall::desktop
{
namespace
{
QByteArray randomBytes(qsizetype size)
{
    QByteArray data(size, Qt::Uninitialized);
    randombytes_buf(data.data(), size_t(size));
    return data;
}
void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace
SecretKey::SecretKey(QByteArray bytes) : bytes_(std::move(bytes))
{
    require(bytes_.size() == crypto_aead_xchacha20poly1305_ietf_KEYBYTES, "Invalid key size");
}
SecretKey::~SecretKey()
{
    CryptoService::wipe(bytes_);
}
CryptoService::CryptoService()
{
    require(sodium_init() >= 0, "Cannot initialize cryptography");
}
void CryptoService::wipe(QByteArray& data)
{
    data.detach();
    sodium_memzero(data.data(), size_t(data.size()));
    data.clear();
}
void CryptoService::wipe(QString& data)
{
    data.detach();
    sodium_memzero(data.data(), size_t(data.size()) * sizeof(QChar));
    data.clear();
}
void CryptoService::validateKeyInfo(const VaultKeyInfo& info)
{
    const auto& k = info.kdf;
    require(k.algorithm == "argon2id" && k.salt.size() == crypto_pwhash_SALTBYTES &&
                k.parallelism == 1 && k.memory >= 8192 && k.memory <= 262144 && k.iterations >= 1 &&
                k.iterations <= 10,
            "Unsupported or unsafe vault KDF parameters");
    require(info.nonce.size() == crypto_aead_xchacha20poly1305_ietf_NPUBBYTES &&
                info.encryptedKey.size() == crypto_aead_xchacha20poly1305_ietf_KEYBYTES +
                                                crypto_aead_xchacha20poly1305_ietf_ABYTES,
            "Invalid encrypted vault key");
}
KeyPtr CryptoService::derive(QString& password, const KdfParameters& kdf)
{
    auto utf8 = password.toUtf8();
    wipe(password);
    QByteArray output(crypto_aead_xchacha20poly1305_ietf_KEYBYTES, Qt::Uninitialized);
    const auto result = crypto_pwhash(
        reinterpret_cast<unsigned char*>(output.data()), size_t(output.size()), utf8.constData(),
        size_t(utf8.size()), reinterpret_cast<const unsigned char*>(kdf.salt.constData()),
        kdf.iterations, size_t(kdf.memory * 1024), crypto_pwhash_ALG_ARGON2ID13);
    wipe(utf8);
    if (result != 0)
    {
        wipe(output);
        throw std::runtime_error("Key derivation failed");
    }
    return std::make_shared<SecretKey>(std::move(output));
}
EncryptedEntry CryptoService::encrypt(const QByteArray& plain, const KeyPtr& key,
                                      const QByteArray& context)
{
    require(bool(key), "Vault is locked");
    EncryptedEntry output{
        QByteArray(plain.size() + crypto_aead_xchacha20poly1305_ietf_ABYTES, Qt::Uninitialized),
        randomBytes(crypto_aead_xchacha20poly1305_ietf_NPUBBYTES)};
    unsigned long long size = 0;
    const int result = crypto_aead_xchacha20poly1305_ietf_encrypt(
        reinterpret_cast<unsigned char*>(output.ciphertext.data()), &size,
        reinterpret_cast<const unsigned char*>(plain.constData()), size_t(plain.size()),
        reinterpret_cast<const unsigned char*>(context.constData()), size_t(context.size()),
        nullptr, reinterpret_cast<const unsigned char*>(output.nonce.constData()),
        reinterpret_cast<const unsigned char*>(key->bytes().constData()));
    require(result == 0, "Encryption failed");
    output.ciphertext.resize(qsizetype(size));
    return output;
}
QByteArray CryptoService::decrypt(const EncryptedEntry& entry, const KeyPtr& key,
                                  const QByteArray& context)
{
    require(bool(key) && entry.nonce.size() == crypto_aead_xchacha20poly1305_ietf_NPUBBYTES &&
                entry.ciphertext.size() >= crypto_aead_xchacha20poly1305_ietf_ABYTES,
            "Invalid encrypted data");
    QByteArray plain(entry.ciphertext.size() - crypto_aead_xchacha20poly1305_ietf_ABYTES,
                     Qt::Uninitialized);
    unsigned long long size = 0;
    const auto result = crypto_aead_xchacha20poly1305_ietf_decrypt(
        reinterpret_cast<unsigned char*>(plain.data()), &size, nullptr,
        reinterpret_cast<const unsigned char*>(entry.ciphertext.constData()),
        size_t(entry.ciphertext.size()),
        reinterpret_cast<const unsigned char*>(context.constData()), size_t(context.size()),
        reinterpret_cast<const unsigned char*>(entry.nonce.constData()),
        reinterpret_cast<const unsigned char*>(key->bytes().constData()));
    if (result != 0)
    {
        wipe(plain);
        throw std::runtime_error("Wrong master password or damaged encrypted data");
    }
    plain.resize(qsizetype(size));
    return plain;
}
VaultKeyInfo CryptoService::createVault(QString password)
{
    VaultKeyInfo info;
    info.kdf.salt = randomBytes(crypto_pwhash_SALTBYTES);
    const auto kek = derive(password, info.kdf);
    const auto key =
        std::make_shared<SecretKey>(randomBytes(crypto_aead_xchacha20poly1305_ietf_KEYBYTES));
    const auto encrypted = encrypt(key->bytes(), kek, "passwall:v1:vault-key");
    info.encryptedKey = encrypted.ciphertext;
    info.nonce = encrypted.nonce;
    return info;
}
KeyPtr CryptoService::unlock(QString password, const VaultKeyInfo& info)
{
    try
    {
        validateKeyInfo(info);
    }
    catch (...)
    {
        wipe(password);
        throw;
    }
    return std::make_shared<SecretKey>(decrypt(
        {info.encryptedKey, info.nonce}, derive(password, info.kdf), "passwall:v1:vault-key"));
}
EncryptedEntry CryptoService::encryptEntry(const QJsonObject& entry, const KeyPtr& key)
{
    auto plain = QJsonDocument(entry).toJson(QJsonDocument::Compact);
    try
    {
        auto result = encrypt(plain, key, "passwall:v1:entry");
        wipe(plain);
        return result;
    }
    catch (...)
    {
        wipe(plain);
        throw;
    }
}
QJsonObject CryptoService::decryptEntry(const EncryptedEntry& entry, const KeyPtr& key)
{
    auto plain = decrypt(entry, key, "passwall:v1:entry");
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(plain, &error);
    wipe(plain);
    require(error.error == QJsonParseError::NoError && document.isObject(), "Invalid vault entry");
    return document.object();
}
} // namespace passwall::desktop

#include "KeyStore.h"
#include "api/ApiClient.h"
#include "crypto/CryptoService.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QStandardPaths>
#include <stdexcept>

namespace passwall::desktop
{
namespace
{
QJsonObject read(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 16384)
        throw std::runtime_error(
            "Encrypted key metadata is unavailable. Import its backup on this device.");
    const auto object = QJsonDocument::fromJson(file.readAll()).object();
    if (object["format"].toString() != "passwall-key-v1")
        throw std::runtime_error("Invalid key backup format");
    return object;
}
void write(const QString& path, const QJsonObject& object)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        throw std::runtime_error("Cannot write encrypted key metadata");
    file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    const auto data = QJsonDocument(object).toJson();
    if (file.write(data) != data.size() || !file.commit())
        throw std::runtime_error("Cannot save encrypted key metadata");
}
VaultKeyInfo validate(const QJsonObject& object, const QUrl& server, const QString& email)
{
    if (object["server"].toString() != server.toString() || object["email"].toString() != email)
        throw std::runtime_error("Key backup belongs to a different server or account");
    auto info = ApiClient::parseKeyInfo(object["key"].toObject());
    CryptoService::validateKeyInfo(info);
    return info;
}
} // namespace
QString KeyStore::path(const QUrl& server, const QString& email)
{
    const auto base = qEnvironmentVariable(
        "PASSWALL_DATA_DIR",
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    const QString directory = base + "/keys";
    if (!QDir().mkpath(directory))
        throw std::runtime_error("Cannot create local key directory");
    return directory + '/' +
           QCryptographicHash::hash(server.toEncoded() + '\n' + email.toUtf8(),
                                    QCryptographicHash::Sha256)
               .toHex() +
           ".json";
}
VaultKeyInfo KeyStore::load(const QUrl& server, const QString& email)
{
    return validate(read(path(server, email)), server, email);
}
void KeyStore::save(const QUrl& server, const QString& email, const VaultKeyInfo& info)
{
    CryptoService::validateKeyInfo(info);
    write(path(server, email), {{"format", "passwall-key-v1"},
                                {"server", server.toString()},
                                {"email", email},
                                {"key", ApiClient::keyInfoJson(info)}});
}
QString KeyStore::stage(const QUrl& server, const QString& email, const VaultKeyInfo& info)
{
    const auto candidate =
        path(server, email) + '.' +
        QCryptographicHash::hash(info.encryptedKey, QCryptographicHash::Sha256).toHex() +
        ".pending.json";
    writeBackup(server, email, info, QUrl::fromLocalFile(candidate));
    return candidate;
}
void KeyStore::exportFile(const QUrl& server, const QString& email, const QUrl& file)
{
    if (!file.isLocalFile())
        throw std::runtime_error("Choose a local backup file");
    const auto object = read(path(server, email));
    validate(object, server, email);
    write(file.toLocalFile(), object);
}
void KeyStore::writeBackup(const QUrl& server, const QString& email, const VaultKeyInfo& info,
                           const QUrl& file)
{
    if (!file.isLocalFile())
        throw std::runtime_error("Choose a local backup file");
    CryptoService::validateKeyInfo(info);
    write(file.toLocalFile(), {{"format", "passwall-key-v1"},
                               {"server", server.toString()},
                               {"email", email},
                               {"key", ApiClient::keyInfoJson(info)}});
}
void KeyStore::importFile(const QUrl& server, const QString& email, const QUrl& file)
{
    if (!file.isLocalFile())
        throw std::runtime_error("Choose a local backup file");
    const auto object = read(file.toLocalFile());
    save(server, email, validate(object, server, email));
}
} // namespace passwall::desktop

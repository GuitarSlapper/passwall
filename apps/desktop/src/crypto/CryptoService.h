#pragma once

#include "api/dto/ApiTypes.h"
#include <QJsonObject>
#include <memory>

namespace passwall::desktop
{
class SecretKey final
{
public:
    explicit SecretKey(QByteArray bytes);
    ~SecretKey();
    SecretKey(const SecretKey&) = delete;
    SecretKey& operator=(const SecretKey&) = delete;
    const QByteArray& bytes() const
    {
        return bytes_;
    }

private:
    QByteArray bytes_;
};
using KeyPtr = std::shared_ptr<SecretKey>;

class CryptoService final
{
public:
    CryptoService();
    static VaultKeyInfo createVault(QString masterPassword);
    static KeyPtr unlock(QString masterPassword, const VaultKeyInfo&);
    static EncryptedEntry encryptEntry(const QJsonObject&, const KeyPtr&);
    static QJsonObject decryptEntry(const EncryptedEntry&, const KeyPtr&);
    static void validateKeyInfo(const VaultKeyInfo&);
    static void wipe(QByteArray&);
    static void wipe(QString&);

private:
    static KeyPtr derive(QString& masterPassword, const KdfParameters&);
    static EncryptedEntry encrypt(const QByteArray&, const KeyPtr&, const QByteArray& context);
    static QByteArray decrypt(const EncryptedEntry&, const KeyPtr&, const QByteArray& context);
};
} // namespace passwall::desktop

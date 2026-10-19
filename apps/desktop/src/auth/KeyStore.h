#pragma once
#include "api/dto/ApiTypes.h"
#include <QUrl>

namespace passwall::desktop
{
class KeyStore final
{
public:
    static VaultKeyInfo load(const QUrl& server, const QString& email);
    static void save(const QUrl& server, const QString& email, const VaultKeyInfo&);
    static QString stage(const QUrl&, const QString&, const VaultKeyInfo&);
    static void exportFile(const QUrl& server, const QString& email, const QUrl& file);
    static void importFile(const QUrl& server, const QString& email, const QUrl& file);
    static void writeBackup(const QUrl&, const QString&, const VaultKeyInfo&, const QUrl& file);

private:
    static QString path(const QUrl&, const QString&);
};
} // namespace passwall::desktop

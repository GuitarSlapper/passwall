#pragma once
#include "VaultEntry.h"
#include "auth/AuthManager.h"
#include "crypto/CryptoService.h"
#include <QVariantList>
namespace passwall::desktop
{
class VaultManager final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool unlocked READ unlocked NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QVariantList entries READ entries NOTIFY changed)
public:
    VaultManager(ApiClient&, AuthManager&, QObject* parent = nullptr);
    ~VaultManager() override;
    bool unlocked() const
    {
        return bool(key_);
    }
    bool busy() const
    {
        return busy_;
    }
    QString message() const
    {
        return message_;
    }
    QVariantList entries() const;
    Q_INVOKABLE QString password(const QString& id) const;
    Q_INVOKABLE void unlock(QString masterPassword);
    Q_INVOKABLE void lock();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void createEntry(QString site, QString username, QString password, QString notes);
    Q_INVOKABLE void deleteEntry(const QString& id);
signals:
    void changed();
    void locked();
    void entryCreated();

private:
    void clearEntries();
    ApiClient& api_;
    AuthManager& auth_;
    KeyPtr key_;
    QVector<VaultEntry> entries_;
    bool busy_{false};
    QString message_;
    quint64 generation_{0};
};
} // namespace passwall::desktop

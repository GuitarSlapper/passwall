#pragma once
#include <QString>
#include <QVariantMap>
namespace passwall::desktop
{
struct VaultEntry
{
    QString id, site, username, password, notes;
    QVariantMap summary() const
    {
        return {{"id", id}, {"site", site}, {"username", username}, {"notes", notes}};
    }
};
} // namespace passwall::desktop

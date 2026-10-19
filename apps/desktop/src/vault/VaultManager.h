// MIT License
//
// Copyright (c) 2026 GuitarSlapper
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

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

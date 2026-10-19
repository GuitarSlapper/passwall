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
#include "api/ApiClient.h"
#include <QObject>

namespace passwall::desktop
{
class AuthManager final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool authenticated READ authenticated NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString email READ email NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QString serverUrl READ serverUrl NOTIFY changed)
public:
    explicit AuthManager(ApiClient&, QObject* parent = nullptr);
    bool authenticated() const
    {
        return authenticated_;
    }
    bool busy() const
    {
        return busy_;
    }
    QString email() const
    {
        return email_;
    }
    QString message() const
    {
        return message_;
    }
    QString serverUrl() const
    {
        return api_.baseUrl().toString();
    }
    VaultKeyInfo keyInfo() const;
    Q_INVOKABLE bool setServerUrl(const QString&);
    Q_INVOKABLE void login(QString email, QString password);
    Q_INVOKABLE void registerUser(QString email, QString password, QString masterPassword);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void exportKey(const QUrl& file);
    Q_INVOKABLE void importKey(const QUrl& file);
signals:
    void changed();
    void sessionChanged();
    void registered();

private:
    void expire();
    ApiClient& api_;
    bool authenticated_{false};
    bool busy_{false};
    QString email_;
    QString message_;
    quint64 generation_{0};
    std::optional<VaultKeyInfo> recoveryInfo_;
    QUrl recoveryServer_;
    QString recoveryEmail_;
};
} // namespace passwall::desktop

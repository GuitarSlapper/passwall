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
#include "dto/ApiTypes.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QUrl>

namespace passwall::desktop
{
class ApiClient final : public QObject
{
    Q_OBJECT
public:
    explicit ApiClient(QObject* parent = nullptr);
    QUrl baseUrl() const
    {
        return baseUrl_;
    }
    bool setBaseUrl(const QUrl&);
    void setToken(QByteArray token);
    void clearToken();
    void login(const LoginRequest&, ApiCallback<LoginResponse>);
    void registerUser(const RegisterRequest&, ApiCallback<RegisterResponse>);
    void logout(ApiCallback<Empty>);
    void getHealth(ApiCallback<HealthResponse>);
    void getReady(ApiCallback<Empty>);
    void getVaultEntries(ApiCallback<QVector<VaultEntryDto>>);
    void createVaultEntry(const EncryptedEntry&, ApiCallback<VaultEntryDto>);
    void deleteVaultEntry(const QString& id, ApiCallback<Empty>);
    static QJsonObject keyInfoJson(const VaultKeyInfo&);
    static VaultKeyInfo parseKeyInfo(const QJsonObject&);
signals:
    void sessionExpired();

private:
    using Reply = std::function<void(ApiResult<QJsonDocument>)>;
    void request(const QByteArray& method, const QString& path, const QJsonObject&,
                 bool authenticated, int expectedStatus, Reply);
    QNetworkAccessManager network_;
    QUrl baseUrl_{QStringLiteral("http://127.0.0.1:8080")};
    QByteArray token_;
    quint64 session_{0};
};
} // namespace passwall::desktop

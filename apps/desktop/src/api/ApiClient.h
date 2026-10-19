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

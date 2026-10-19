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

#include "ApiClient.h"
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUuid>
#include <cmath>
#include <stdexcept>

namespace passwall::desktop
{
namespace
{
QString string(const QJsonObject& o, const char* key)
{
    if (!o[key].isString())
        throw std::runtime_error("Invalid server response");
    return o[key].toString();
}
QByteArray binary(const QJsonObject& o, const char* key)
{
    const auto text = string(o, key).toLatin1();
    const auto decoded =
        QByteArray::fromBase64Encoding(text, QByteArray::AbortOnBase64DecodingErrors);
    if (!decoded)
        throw std::runtime_error("Invalid Base64 data");
    return decoded.decoded;
}
quint64 integer(const QJsonObject& o, const char* key)
{
    const auto value = o[key].toDouble(-1);
    if (!o[key].isDouble() || value < 1 || value > 9007199254740991.0 || std::floor(value) != value)
        throw std::runtime_error("Invalid KDF parameters");
    return quint64(value);
}
VaultEntryDto entry(const QJsonObject& o)
{
    VaultEntryDto value{string(o, "id"),
                        {binary(o, "encrypted_data"), binary(o, "nonce")},
                        QDateTime::fromString(string(o, "created_at"), Qt::ISODateWithMs),
                        QDateTime::fromString(string(o, "updated_at"), Qt::ISODateWithMs)};
    if (QUuid(value.id).isNull() || !value.createdAt.isValid() || !value.updatedAt.isValid())
        throw std::runtime_error("Invalid vault entry response");
    return value;
}
template <class T, class F> auto convert(ApiCallback<T> done, F parse)
{
    return [done = std::move(done), parse](ApiResult<QJsonDocument> result)
    {
        if (!result)
        {
            done({{}, result.error});
            return;
        }
        try
        {
            done({parse(*result.value), {}});
        }
        catch (const std::exception& e)
        {
            done({{}, QString::fromUtf8(e.what())});
        }
    };
}
} // namespace
ApiClient::ApiClient(QObject* parent) : QObject(parent) {}
bool ApiClient::setBaseUrl(const QUrl& url)
{
    const bool local =
        url.host() == "127.0.0.1" || url.host() == "localhost" || url.host() == "::1";
    if (!url.isValid() || url.host().isEmpty() || !url.userInfo().isEmpty() || url.hasQuery() ||
        url.hasFragment() || !(url.scheme() == "https" || (url.scheme() == "http" && local)))
        return false;
    baseUrl_ = url.adjusted(QUrl::StripTrailingSlash);
    clearToken();
    return true;
}
void ApiClient::setToken(QByteArray token)
{
    token_ = std::move(token);
    ++session_;
}
void ApiClient::clearToken()
{
    token_.fill('\0');
    token_.clear();
    ++session_;
}
void ApiClient::request(const QByteArray& method, const QString& path, const QJsonObject& body,
                        bool authenticated, int expected, Reply done)
{
    QUrl url = baseUrl_;
    url.setPath(baseUrl_.path() + path);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    request.setTransferTimeout(15000);
    if (authenticated)
        request.setRawHeader("Authorization", "Bearer " + token_);
    const auto generation = session_;
    auto* reply = network_.sendCustomRequest(
        request, method,
        body.isEmpty() ? QByteArray{} : QJsonDocument(body).toJson(QJsonDocument::Compact));
    auto responseBytes = std::make_shared<QByteArray>();
    connect(reply, &QNetworkReply::readyRead, reply,
            [reply, responseBytes]
            {
                responseBytes->append(reply->readAll());
                if (responseBytes->size() > 4 * 1024 * 1024)
                    reply->abort();
            });
    connect(reply, &QNetworkReply::downloadProgress, reply,
            [reply](qint64 received, qint64 total)
            {
                if (received > 4 * 1024 * 1024 || total > 4 * 1024 * 1024)
                    reply->abort();
            });
    connect(
        reply, &QNetworkReply::finished, this,
        [this, reply, responseBytes, authenticated, generation, expected, done = std::move(done)]
        {
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            responseBytes->append(reply->readAll());
            const auto& bytes = *responseBytes;
            const auto networkError = reply->error();
            reply->deleteLater();
            if (authenticated && generation != session_)
            {
                done({{}, "Session changed"});
                return;
            }
            if (status == 401 && authenticated)
                emit sessionExpired();
            if (status != expected || networkError != QNetworkReply::NoError)
            {
                QString message;
                switch (status)
                {
                case 400:
                    message = "Invalid request";
                    break;
                case 401:
                    message = "Invalid credentials or expired session";
                    break;
                case 404:
                    message = "Entry not found";
                    break;
                case 409:
                    message = "An account with this email already exists";
                    break;
                default:
                    message = status
                                  ? QStringLiteral("Server request failed (%1)").arg(status)
                                  : QStringLiteral(
                                        "Cannot reach server. Check connection and TLS settings.");
                }
                done({{}, message});
                return;
            }
            if (expected == 204)
            {
                done({QJsonDocument{}, {}});
                return;
            }
            QJsonParseError error;
            const auto document = QJsonDocument::fromJson(bytes, &error);
            if (error.error != QJsonParseError::NoError || document.isNull())
                done({{}, "Invalid server response"});
            else
                done({document, {}});
        });
}
QJsonObject ApiClient::keyInfoJson(const VaultKeyInfo& info)
{
    return {{"kdf", QJsonObject{{"algorithm", info.kdf.algorithm},
                                {"salt", QString::fromLatin1(info.kdf.salt.toBase64())},
                                {"memory", double(info.kdf.memory)},
                                {"iterations", double(info.kdf.iterations)},
                                {"parallelism", double(info.kdf.parallelism)}}},
            {"encrypted_vault_key", QString::fromLatin1(info.encryptedKey.toBase64())},
            {"vault_key_nonce", QString::fromLatin1(info.nonce.toBase64())}};
}
VaultKeyInfo ApiClient::parseKeyInfo(const QJsonObject& o)
{
    const auto k = o["kdf"].toObject();
    return {{string(k, "algorithm"), binary(k, "salt"), integer(k, "memory"),
             integer(k, "iterations"), integer(k, "parallelism")},
            binary(o, "encrypted_vault_key"),
            binary(o, "vault_key_nonce")};
}
void ApiClient::login(const LoginRequest& input, ApiCallback<LoginResponse> done)
{
    request("POST", "/login", {{"email", input.email}, {"password", input.password}}, false, 200,
            convert<LoginResponse>(std::move(done),
                                   [](const QJsonDocument& d)
                                   {
                                       const auto o = d.object();
                                       const auto token = string(o, "access_token").toLatin1();
                                       if (string(o, "token_type") != "Bearer" || token.isEmpty() ||
                                           token.contains('\r') || token.contains('\n'))
                                           throw std::runtime_error("Invalid token");
                                       return LoginResponse{token};
                                   }));
}
void ApiClient::registerUser(const RegisterRequest& input, ApiCallback<RegisterResponse> done)
{
    auto body = keyInfoJson(input.keyInfo);
    body["email"] = input.email;
    body["auth_password"] = input.authPassword;
    request("POST", "/register", body, false, 201,
            convert<RegisterResponse>(std::move(done),
                                      [](const QJsonDocument& d)
                                      {
                                          auto value =
                                              RegisterResponse{string(d.object(), "id"),
                                                               string(d.object(), "email")};
                                          if (QUuid(value.id).isNull())
                                              throw std::runtime_error("Invalid user ID");
                                          return value;
                                      }));
}
void ApiClient::logout(ApiCallback<Empty> done)
{
    request("POST", "/logout", {}, true, 204,
            convert<Empty>(std::move(done), [](const QJsonDocument&) { return Empty{}; }));
}
void ApiClient::getHealth(ApiCallback<HealthResponse> done)
{
    request("GET", "/health", {}, false, 200,
            convert<HealthResponse>(std::move(done),
                                    [](const QJsonDocument& d)
                                    {
                                        return HealthResponse{string(d.object(), "status"),
                                                              string(d.object(), "version")};
                                    }));
}
void ApiClient::getReady(ApiCallback<Empty> done)
{
    request("GET", "/ready", {}, false, 200,
            convert<Empty>(std::move(done),
                           [](const QJsonDocument& d)
                           {
                               if (string(d.object(), "status") != "ready")
                                   throw std::runtime_error("Not ready");
                               return Empty{};
                           }));
}
void ApiClient::getVaultEntries(ApiCallback<QVector<VaultEntryDto>> done)
{
    request("GET", "/vault/entries", {}, true, 200,
            convert<QVector<VaultEntryDto>>(std::move(done),
                                            [](const QJsonDocument& d)
                                            {
                                                if (!d.isArray())
                                                    throw std::runtime_error("Expected array");
                                                QVector<VaultEntryDto> values;
                                                for (const auto& v : d.array())
                                                    values.push_back(entry(v.toObject()));
                                                return values;
                                            }));
}
void ApiClient::createVaultEntry(const EncryptedEntry& input, ApiCallback<VaultEntryDto> done)
{
    request("POST", "/vault/entries",
            {{"encrypted_data", QString::fromLatin1(input.ciphertext.toBase64())},
             {"nonce", QString::fromLatin1(input.nonce.toBase64())}},
            true, 201,
            convert<VaultEntryDto>(std::move(done),
                                   [](const QJsonDocument& d) { return entry(d.object()); }));
}
void ApiClient::deleteVaultEntry(const QString& id, ApiCallback<Empty> done)
{
    if (QUuid(id).isNull())
    {
        done({{}, "Invalid entry ID"});
        return;
    }
    request("DELETE", "/vault/entries/" + id, {}, true, 204,
            convert<Empty>(std::move(done), [](const QJsonDocument&) { return Empty{}; }));
}
} // namespace passwall::desktop

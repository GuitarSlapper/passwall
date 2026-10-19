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

#include "AuthManager.h"
#include "KeyStore.h"
#include "crypto/CryptoService.h"
#include <QFile>
#include <QFutureWatcher>
#include <QPointer>
#include <QtConcurrent/QtConcurrentRun>

namespace passwall::desktop
{
namespace
{
bool credentials(const QString& email, const QString& password)
{
    return email.contains('@') && !email.contains(' ') && password.toUcs4().size() >= 8;
}
struct Generated
{
    std::optional<VaultKeyInfo> info;
    QString error;
};
} // namespace
AuthManager::AuthManager(ApiClient& api, QObject* parent) : QObject(parent), api_(api)
{
    connect(&api_, &ApiClient::sessionExpired, this, &AuthManager::expire);
}
bool AuthManager::setServerUrl(const QString& url)
{
    if (busy_ || authenticated_)
        return false;
    const bool valid = api_.setBaseUrl(QUrl(url.trimmed()));
    message_ =
        valid ? QString{} : QStringLiteral("Use HTTPS, or HTTP on localhost for development.");
    emit changed();
    return valid;
}
void AuthManager::login(QString email, QString password)
{
    if (busy_ || authenticated_)
        return;
    email = email.trimmed();
    if (!credentials(email, password))
    {
        message_ = "Enter an email and a password of at least 8 characters.";
        emit changed();
        return;
    }
    busy_ = true;
    message_.clear();
    emit changed();
    const auto generation = ++generation_;
    api_.login(
        {email, password},
        [guard = QPointer<AuthManager>(this), generation, email](ApiResult<LoginResponse> result)
        {
            if (!guard || guard->generation_ != generation)
                return;
            guard->busy_ = false;
            if (result)
            {
                guard->api_.setToken(result.value->accessToken);
                guard->email_ = email;
                guard->authenticated_ = true;
                emit guard->sessionChanged();
            }
            else
                guard->message_ = result.error;
            emit guard->changed();
        });
    CryptoService::wipe(password);
}
void AuthManager::registerUser(QString email, QString password, QString masterPassword)
{
    if (busy_ || authenticated_)
        return;
    email = email.trimmed();
    if (!credentials(email, password) || masterPassword.toUcs4().size() < 8)
    {
        message_ = "Enter an email and two passwords of at least 8 characters.";
        emit changed();
        return;
    }
    busy_ = true;
    message_.clear();
    emit changed();
    const auto generation = ++generation_;
    auto* watcher = new QFutureWatcher<Generated>(this);
    connect(
        watcher, &QFutureWatcher<Generated>::finished, this,
        [this, watcher, generation, email, password = std::move(password)]() mutable
        {
            const auto generated = watcher->result();
            watcher->deleteLater();
            if (generation != generation_)
            {
                CryptoService::wipe(password);
                return;
            }
            if (!generated.info)
            {
                busy_ = false;
                message_ = generated.error;
                CryptoService::wipe(password);
                emit changed();
                return;
            }
            recoveryInfo_ = generated.info;
            recoveryServer_ = api_.baseUrl();
            recoveryEmail_ = email;
            email_ = email;
            QString pendingFile;
            try
            {
                pendingFile = KeyStore::stage(api_.baseUrl(), email, *generated.info);
            }
            catch (const std::exception& e)
            {
                busy_ = false;
                message_ = QString::fromUtf8(e.what());
                CryptoService::wipe(password);
                emit changed();
                return;
            }
            api_.registerUser(
                {email, password, *generated.info},
                [guard = QPointer<AuthManager>(this), generation, email, pendingFile,
                 info = *generated.info](ApiResult<RegisterResponse> result)
                {
                    if (!guard || generation != guard->generation_)
                        return;
                    guard->busy_ = false;
                    if (result)
                    {
                        guard->email_ = email;
                        try
                        {
                            KeyStore::save(guard->api_.baseUrl(), email, info);
                            QFile::remove(pendingFile);
                            guard->message_ =
                                "Account created. Sign in, then export your encrypted key backup.";
                        }
                        catch (const std::exception&)
                        {
                            guard->message_ =
                                "Account created, but local key storage failed. Export your key "
                                "backup before closing this application.";
                        }
                        emit guard->registered();
                    }
                    else
                        guard->message_ = result.error +
                                          ". If the response was lost, sign in to check the "
                                          "account. Candidate key backup: " +
                                          pendingFile;
                    emit guard->changed();
                });
            CryptoService::wipe(password);
        });
    watcher->setFuture(QtConcurrent::run(
        [master = std::move(masterPassword)]() mutable
        {
            try
            {
                return Generated{CryptoService::createVault(std::move(master)), {}};
            }
            catch (const std::exception& e)
            {
                return Generated{{}, QString::fromUtf8(e.what())};
            }
        }));
}
void AuthManager::expire()
{
    ++generation_;
    api_.clearToken();
    authenticated_ = false;
    busy_ = false;
    message_ = "Session expired. Sign in again.";
    emit sessionChanged();
    emit changed();
}
void AuthManager::logout()
{
    if (!authenticated_ || busy_)
        return;
    busy_ = true;
    emit changed();
    api_.logout(
        [guard = QPointer<AuthManager>(this)](ApiResult<Empty> result)
        {
            if (!guard)
                return;
            guard->api_.clearToken();
            ++guard->generation_;
            guard->authenticated_ = false;
            guard->busy_ = false;
            guard->message_ = result ? QString{}
                                     : "Signed out locally. Server logout failed; the session will "
                                       "expire automatically.";
            emit guard->sessionChanged();
            emit guard->changed();
        });
    emit sessionChanged();
}
void AuthManager::exportKey(const QUrl& file)
{
    try
    {
        KeyStore::writeBackup(api_.baseUrl(), email_, keyInfo(), file);
        message_ = "Encrypted key backup exported.";
    }
    catch (const std::exception& e)
    {
        message_ = QString::fromUtf8(e.what());
    }
    emit changed();
}
VaultKeyInfo AuthManager::keyInfo() const
{
    try
    {
        return KeyStore::load(api_.baseUrl(), email_);
    }
    catch (...)
    {
        if (recoveryInfo_ && recoveryServer_ == api_.baseUrl() && recoveryEmail_ == email_)
            return *recoveryInfo_;
        throw;
    }
}
void AuthManager::importKey(const QUrl& file)
{
    if (!authenticated_ || busy_)
        return;
    emit sessionChanged();
    try
    {
        KeyStore::importFile(api_.baseUrl(), email_, file);
        message_ = "Encrypted key imported. Unlock your vault.";
    }
    catch (const std::exception& e)
    {
        message_ = QString::fromUtf8(e.what());
    }
    emit changed();
}
} // namespace passwall::desktop

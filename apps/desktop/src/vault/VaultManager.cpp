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

#include "VaultManager.h"
#include "auth/KeyStore.h"
#include <QFutureWatcher>
#include <QPointer>
#include <QtConcurrent/QtConcurrentRun>
#include <stdexcept>
namespace passwall::desktop
{
namespace
{
struct UnlockResult
{
    KeyPtr key;
    QString error;
};
} // namespace
VaultManager::VaultManager(ApiClient& api, AuthManager& auth, QObject* parent)
    : QObject(parent), api_(api), auth_(auth)
{
    connect(&auth_, &AuthManager::sessionChanged, this, &VaultManager::lock);
}
VaultManager::~VaultManager()
{
    clearEntries();
}
void VaultManager::clearEntries()
{
    for (auto& entry : entries_)
    {
        CryptoService::wipe(entry.site);
        CryptoService::wipe(entry.username);
        CryptoService::wipe(entry.password);
        CryptoService::wipe(entry.notes);
    }
    entries_.clear();
}
QVariantList VaultManager::entries() const
{
    QVariantList list;
    for (const auto& entry : entries_)
        list.append(entry.summary());
    return list;
}
QString VaultManager::password(const QString& id) const
{
    if (key_)
        for (const auto& entry : entries_)
            if (entry.id == id)
                return entry.password;
    return {};
}
void VaultManager::lock()
{
    ++generation_;
    key_.reset();
    clearEntries();
    busy_ = false;
    message_.clear();
    emit locked();
    emit changed();
}
void VaultManager::unlock(QString password)
{
    if (!auth_.authenticated() || auth_.busy() || busy_ || key_)
        return;
    VaultKeyInfo info;
    try
    {
        info = auth_.keyInfo();
    }
    catch (const std::exception& e)
    {
        CryptoService::wipe(password);
        message_ = QString::fromUtf8(e.what());
        emit changed();
        return;
    }
    busy_ = true;
    message_.clear();
    emit changed();
    const auto generation = ++generation_;
    auto* watcher = new QFutureWatcher<UnlockResult>(this);
    connect(watcher, &QFutureWatcher<UnlockResult>::finished, this,
            [this, watcher, generation]
            {
                const auto result = watcher->result();
                watcher->deleteLater();
                if (generation != generation_)
                    return;
                busy_ = false;
                key_ = result.key;
                message_ = result.error;
                emit changed();
                if (key_)
                    refresh();
            });
    watcher->setFuture(QtConcurrent::run(
        [password = std::move(password), info]() mutable
        {
            try
            {
                return UnlockResult{CryptoService::unlock(std::move(password), info), {}};
            }
            catch (const std::exception& e)
            {
                return UnlockResult{{}, QString::fromUtf8(e.what())};
            }
        }));
}
void VaultManager::refresh()
{
    if (!key_ || busy_ || !auth_.authenticated() || auth_.busy())
        return;
    busy_ = true;
    message_.clear();
    emit changed();
    const auto generation = generation_;
    api_.getVaultEntries(
        [guard = QPointer<VaultManager>(this), generation](ApiResult<QVector<VaultEntryDto>> result)
        {
            if (!guard || generation != guard->generation_)
                return;
            guard->busy_ = false;
            if (!result)
            {
                guard->message_ = result.error;
                emit guard->changed();
                return;
            }
            QVector<VaultEntry> decrypted;
            try
            {
                for (const auto& dto : *result.value)
                {
                    const auto object = CryptoService::decryptEntry(dto.payload, guard->key_);
                    for (const auto field : {"site", "username", "password", "notes"})
                        if (!object[field].isString())
                            throw std::runtime_error("Invalid encrypted entry format");
                    decrypted.push_back(
                        {dto.id, object["site"].toString(), object["username"].toString(),
                         object["password"].toString(), object["notes"].toString()});
                }
                guard->clearEntries();
                guard->entries_ = std::move(decrypted);
            }
            catch (const std::exception& e)
            {
                guard->message_ = QString::fromUtf8(e.what());
            }
            emit guard->changed();
        });
}
void VaultManager::createEntry(QString site, QString username, QString password, QString notes)
{
    if (!key_ || busy_ || !auth_.authenticated() || auth_.busy())
        return;
    if (site.trimmed().isEmpty())
    {
        message_ = "Enter a site or title.";
        emit changed();
        return;
    }
    EncryptedEntry encrypted;
    try
    {
        encrypted = CryptoService::encryptEntry(
            {{"site", site}, {"username", username}, {"password", password}, {"notes", notes}},
            key_);
    }
    catch (const std::exception& e)
    {
        message_ = QString::fromUtf8(e.what());
        emit changed();
        return;
    }
    CryptoService::wipe(password);
    busy_ = true;
    message_.clear();
    emit changed();
    const auto generation = generation_;
    api_.createVaultEntry(
        encrypted,
        [guard = QPointer<VaultManager>(this), generation](ApiResult<VaultEntryDto> result)
        {
            if (!guard || generation != guard->generation_)
                return;
            guard->busy_ = false;
            if (result)
            {
                emit guard->entryCreated();
                guard->refresh();
            }
            else
            {
                guard->message_ = result.error;
                emit guard->changed();
            }
        });
}
void VaultManager::deleteEntry(const QString& id)
{
    if (!key_ || busy_ || !auth_.authenticated() || auth_.busy())
        return;
    busy_ = true;
    message_.clear();
    emit changed();
    const auto generation = generation_;
    api_.deleteVaultEntry(
        id,
        [guard = QPointer<VaultManager>(this), generation](ApiResult<Empty> result)
        {
            if (!guard || generation != guard->generation_)
                return;
            guard->busy_ = false;
            if (result)
                guard->refresh();
            else
            {
                guard->message_ = result.error;
                emit guard->changed();
            }
        });
}
} // namespace passwall::desktop

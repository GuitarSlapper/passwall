#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QVector>
#include <functional>
#include <optional>

namespace passwall::desktop
{
struct KdfParameters
{
    QString algorithm{"argon2id"};
    QByteArray salt;
    quint64 memory{65536};
    quint64 iterations{3};
    quint64 parallelism{1};
};
struct VaultKeyInfo
{
    KdfParameters kdf;
    QByteArray encryptedKey;
    QByteArray nonce;
};
struct LoginRequest
{
    QString email;
    QString password;
};
struct LoginResponse
{
    QByteArray accessToken;
};
struct RegisterRequest
{
    QString email;
    QString authPassword;
    VaultKeyInfo keyInfo;
};
struct RegisterResponse
{
    QString id;
    QString email;
};
struct EncryptedEntry
{
    QByteArray ciphertext;
    QByteArray nonce;
};
struct VaultEntryDto
{
    QString id;
    EncryptedEntry payload;
    QDateTime createdAt;
    QDateTime updatedAt;
};
struct HealthResponse
{
    QString status;
    QString version;
};
struct Empty
{
};
template <class T> struct ApiResult
{
    std::optional<T> value;
    QString error;
    explicit operator bool() const
    {
        return value.has_value();
    }
};
template <class T> using ApiCallback = std::function<void(ApiResult<T>)>;
} // namespace passwall::desktop

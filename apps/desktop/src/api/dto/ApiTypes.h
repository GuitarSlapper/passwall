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

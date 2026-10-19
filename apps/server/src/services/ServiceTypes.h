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

#include <json/json.h>

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace passwall::server
{
struct RegisterRequest
{
    std::string email;
    std::string authPassword;
    Json::Value kdf;
    std::string encryptedVaultKey;
    std::string vaultKeyNonce;
};

struct RegisteredUser
{
    std::string id; // UUID
    std::string email;
};

struct VaultEntry
{
    std::string id; // UUID
    std::string encryptedData;
    std::string nonce;
    std::string createdAt; // RFC 3339, UTC
    std::string updatedAt; // RFC 3339, UTC
};

enum class ServiceError
{
    none,
    emailAlreadyExists,
    invalidCredentials,
    notFound,
    unavailable,
    internalError
};

template <typename T> struct Result
{
    T value{};
    ServiceError error{ServiceError::none};

    static Result success(T value = {})
    {
        return {std::move(value), ServiceError::none};
    }

    static Result failure(ServiceError error)
    {
        return {{}, error};
    }
};

struct Empty
{
};

template <typename T> using Completion = std::function<void(Result<T>)>;

} // namespace passwall::server

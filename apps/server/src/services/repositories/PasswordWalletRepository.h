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

#include "services/ServiceTypes.h"
#include <chrono>
#include <optional>

namespace passwall::server
{
// Persistence input deliberately contains only a password hash.
struct NewUser
{
    std::string id;
    std::string email;
    std::string passwordHash;
    Json::Value kdf;
    std::string encryptedVaultKey;
    std::string vaultKeyNonce;
};

struct UserCredentials
{
    std::string id;
    std::string passwordHash;
};

// Synchronous data access contract. Services call it only on worker threads.
// Implementations must be thread-safe. Database errors propagate to the service
// executor; no HTTP codes, JWT parsing or password hashing belong in this layer.
class PasswordWalletRepository
{
public:
    virtual ~PasswordWalletRepository() = default;
    virtual void checkReady() = 0;
    // Returns false only for a duplicate email; uniqueness is enforced atomically.
    virtual bool createUser(const NewUser&) = 0;
    virtual std::optional<UserCredentials> findCredentials(const std::string& email) = 0;
    virtual void createSession(const std::string& id, const std::string& userId,
                               std::chrono::system_clock::time_point expiresAt) = 0;
    virtual bool hasSession(const std::string& id, const std::string& userId) = 0;
    virtual bool deleteSession(const std::string& id, const std::string& userId) = 0;
    virtual std::vector<VaultEntry> getEntries(const std::string& userId) = 0;
    virtual VaultEntry createEntry(const std::string& id, const std::string& userId,
                                   const std::string& encryptedData, const std::string& nonce) = 0;
    // Always scope access to userId; absent and foreign entries both return false.
    virtual bool deleteEntry(const std::string& id, const std::string& userId) = 0;
};
} // namespace passwall::server

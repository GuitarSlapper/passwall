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

#include "PasswordWalletRepository.h"
#include <drogon/orm/DbClient.h>

namespace passwall::server
{
class PostgresPasswordWalletRepository final : public PasswordWalletRepository
{
public:
    explicit PostgresPasswordWalletRepository(drogon::orm::DbClientPtr db);
    void checkReady() override;
    bool createUser(const NewUser&) override;
    std::optional<UserCredentials> findCredentials(const std::string& email) override;
    void createSession(const std::string& id, const std::string& userId,
                       std::chrono::system_clock::time_point expiresAt) override;
    bool hasSession(const std::string& id, const std::string& userId) override;
    bool deleteSession(const std::string& id, const std::string& userId) override;
    std::vector<VaultEntry> getEntries(const std::string& userId) override;
    VaultEntry createEntry(const std::string& id, const std::string& userId,
                           const std::string& encryptedData, const std::string& nonce) override;
    bool deleteEntry(const std::string& id, const std::string& userId) override;

private:
    drogon::orm::DbClientPtr db_;
};
} // namespace passwall::server

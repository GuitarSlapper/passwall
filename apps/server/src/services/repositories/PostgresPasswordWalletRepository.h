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

#pragma once

#include "services/ServiceTypes.h"
#include <chrono>
#include <optional>

namespace passwall
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
    virtual bool createUser(const NewUser &) = 0;
    virtual std::optional<UserCredentials> findCredentials(const std::string &email) = 0;
    virtual void createSession(const std::string &id, const std::string &userId,
                               std::chrono::system_clock::time_point expiresAt) = 0;
    virtual bool hasSession(const std::string &id, const std::string &userId) = 0;
    virtual bool deleteSession(const std::string &id, const std::string &userId) = 0;
    virtual std::vector<VaultEntry> getEntries(const std::string &userId) = 0;
    virtual VaultEntry createEntry(const std::string &id, const std::string &userId,
                                   const std::string &encryptedData, const std::string &nonce) = 0;
    // Always scope access to userId; absent and foreign entries both return false.
    virtual bool deleteEntry(const std::string &id, const std::string &userId) = 0;
};
}  // namespace passwall

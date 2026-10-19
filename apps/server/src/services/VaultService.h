#pragma once

#include "ServiceTypes.h"
#include <memory>

namespace passwall::server
{
class PasswordWalletRepository;
class AsyncExecutor;
class SessionAuthenticator;

class VaultService final
{
public:
    VaultService(std::shared_ptr<PasswordWalletRepository>, std::shared_ptr<AsyncExecutor>,
                 std::shared_ptr<SessionAuthenticator>);
    void getVaultEntries(std::string token, Completion<std::vector<VaultEntry>>) const;
    void createVaultEntry(std::string token, std::string encryptedData, std::string nonce,
                          Completion<VaultEntry>) const;
    void deleteVaultEntry(std::string token, std::string id, Completion<Empty>) const;

private:
    std::shared_ptr<PasswordWalletRepository> repository_;
    std::shared_ptr<AsyncExecutor> executor_;
    std::shared_ptr<SessionAuthenticator> authenticator_;
};
} // namespace passwall::server

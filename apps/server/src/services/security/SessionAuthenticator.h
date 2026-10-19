#pragma once

#include "SecurityService.h"
#include <memory>

namespace passwall::server
{
class PasswordWalletRepository;

// Combines cryptographic validation with persisted session revocation checks.
// Call on a worker thread: repository operations are synchronous.
class SessionAuthenticator final
{
public:
    SessionAuthenticator(std::shared_ptr<PasswordWalletRepository> repository,
                         std::shared_ptr<SecurityService> security);
    Identity authenticate(const std::string& token) const;

private:
    std::shared_ptr<PasswordWalletRepository> repository_;
    std::shared_ptr<SecurityService> security_;
};
} // namespace passwall::server

#pragma once

#include "ServiceTypes.h"
#include <memory>

namespace passwall
{
class PasswordWalletRepository;
class AsyncExecutor;
class SecurityService;
class SessionAuthenticator;

class AuthenticationService final
{
  public:
    AuthenticationService(std::shared_ptr<PasswordWalletRepository>, std::shared_ptr<AsyncExecutor>,
                          std::shared_ptr<SecurityService>, std::shared_ptr<SessionAuthenticator>);
    void registerUser(RegisterRequest, Completion<RegisteredUser>) const;
    void login(std::string email, std::string password, Completion<std::string>) const;
    void logout(std::string token, Completion<Empty>) const;

  private:
    std::shared_ptr<PasswordWalletRepository> repository_;
    std::shared_ptr<AsyncExecutor> executor_;
    std::shared_ptr<SecurityService> security_;
    std::shared_ptr<SessionAuthenticator> authenticator_;
};
}  // namespace passwall

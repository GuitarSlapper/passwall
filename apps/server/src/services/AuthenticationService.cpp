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

#include "AuthenticationService.h"
#include "AsyncExecutor.h"
#include "repositories/PasswordWalletRepository.h"
#include "security/SessionAuthenticator.h"

#include <stdexcept>

namespace passwall::server
{
AuthenticationService::AuthenticationService(std::shared_ptr<PasswordWalletRepository> repository,
                                             std::shared_ptr<AsyncExecutor> executor,
                                             std::shared_ptr<SecurityService> security,
                                             std::shared_ptr<SessionAuthenticator> authenticator)
    : repository_(std::move(repository)), executor_(std::move(executor)),
      security_(std::move(security)), authenticator_(std::move(authenticator))
{
    if (!repository_ || !executor_ || !security_ || !authenticator_)
        throw std::invalid_argument("Authentication dependencies are required");
}

void AuthenticationService::registerUser(RegisterRequest request,
                                         Completion<RegisteredUser> done) const
{
    executor_->submit<RegisteredUser>(
        std::move(done),
        [repository = repository_, request = std::move(request)]() mutable
        {
            const WipeOnExit wipe{request.authPassword};
            const NewUser user{SecurityService::uuid(),
                               request.email,
                               SecurityService::hashPassword(request.authPassword),
                               request.kdf,
                               request.encryptedVaultKey,
                               request.vaultKeyNonce};
            if (!repository->createUser(user))
                return Result<RegisteredUser>::failure(ServiceError::emailAlreadyExists);
            return Result<RegisteredUser>::success({user.id, user.email});
        });
}

void AuthenticationService::login(std::string email, std::string password,
                                  Completion<std::string> done) const
{
    executor_->submit<std::string>(
        std::move(done),
        [repository = repository_, security = security_, email = std::move(email),
         password = std::move(password)]() mutable
        {
            const WipeOnExit wipe{password};
            const auto user = repository->findCredentials(email);
            const bool valid = security->verifyPassword(user ? user->passwordHash : "", password);
            if (!valid || !user)
                return Result<std::string>::failure(ServiceError::invalidCredentials);
            const auto issued = security->issueToken(user->id);
            repository->createSession(issued.sessionId, user->id, issued.expiresAt);
            return Result<std::string>::success(issued.token);
        });
}

void AuthenticationService::logout(std::string token, Completion<Empty> done) const
{
    executor_->submit<Empty>(
        std::move(done),
        [repository = repository_, authenticator = authenticator_, token = std::move(token)]
        {
            const auto identity = authenticator->authenticate(token);
            if (identity.userId.empty() ||
                !repository->deleteSession(identity.sessionId, identity.userId))
                return Result<Empty>::failure(ServiceError::invalidCredentials);
            return Result<Empty>::success();
        });
}
} // namespace passwall::server

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

#include "VaultService.h"
#include "AsyncExecutor.h"
#include "repositories/PasswordWalletRepository.h"
#include "security/SessionAuthenticator.h"

#include <stdexcept>

namespace passwall::server
{
VaultService::VaultService(std::shared_ptr<PasswordWalletRepository> repository,
                           std::shared_ptr<AsyncExecutor> executor,
                           std::shared_ptr<SessionAuthenticator> authenticator)
    : repository_(std::move(repository)), executor_(std::move(executor)),
      authenticator_(std::move(authenticator))
{
    if (!repository_ || !executor_ || !authenticator_)
        throw std::invalid_argument("Vault dependencies are required");
}

void VaultService::getVaultEntries(std::string token,
                                   Completion<std::vector<VaultEntry>> done) const
{
    executor_->submit<std::vector<VaultEntry>>(
        std::move(done),
        [repository = repository_, authenticator = authenticator_, token = std::move(token)]
        {
            const auto identity = authenticator->authenticate(token);
            if (identity.userId.empty())
                return Result<std::vector<VaultEntry>>::failure(ServiceError::invalidCredentials);
            return Result<std::vector<VaultEntry>>::success(
                repository->getEntries(identity.userId));
        });
}

void VaultService::createVaultEntry(std::string token, std::string encryptedData, std::string nonce,
                                    Completion<VaultEntry> done) const
{
    executor_->submit<VaultEntry>(
        std::move(done),
        [repository = repository_, authenticator = authenticator_, token = std::move(token),
         encryptedData = std::move(encryptedData), nonce = std::move(nonce)]
        {
            const auto identity = authenticator->authenticate(token);
            if (identity.userId.empty())
                return Result<VaultEntry>::failure(ServiceError::invalidCredentials);
            return Result<VaultEntry>::success(repository->createEntry(
                SecurityService::uuid(), identity.userId, encryptedData, nonce));
        });
}

void VaultService::deleteVaultEntry(std::string token, std::string id, Completion<Empty> done) const
{
    executor_->submit<Empty>(
        std::move(done),
        [repository = repository_, authenticator = authenticator_, token = std::move(token),
         id = std::move(id)]
        {
            const auto identity = authenticator->authenticate(token);
            if (identity.userId.empty())
                return Result<Empty>::failure(ServiceError::invalidCredentials);
            if (!SecurityService::isUuid(id) || !repository->deleteEntry(id, identity.userId))
                return Result<Empty>::failure(ServiceError::notFound);
            return Result<Empty>::success();
        });
}
} // namespace passwall::server

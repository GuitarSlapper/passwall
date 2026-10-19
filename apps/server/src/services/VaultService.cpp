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

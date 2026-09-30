#include "HealthService.h"
#include "AsyncExecutor.h"
#include "repositories/PasswordWalletRepository.h"

#include <stdexcept>

namespace passwall
{
HealthService::HealthService(std::shared_ptr<PasswordWalletRepository> repository,
                             std::shared_ptr<AsyncExecutor> executor)
    : repository_(std::move(repository)), executor_(std::move(executor))
{
    if (!repository_ || !executor_) throw std::invalid_argument("Health dependencies are required");
}

void HealthService::checkReady(Completion<Empty> done) const
{
    executor_->submit<Empty>(std::move(done), [repository = repository_] {
        repository->checkReady();
        return Result<Empty>::success();
    });
}
}  // namespace passwall

#pragma once

#include "ServiceTypes.h"
#include <memory>

namespace passwall::server
{
class PasswordWalletRepository;
class AsyncExecutor;

class HealthService final
{
public:
    HealthService(std::shared_ptr<PasswordWalletRepository>, std::shared_ptr<AsyncExecutor>);
    void checkReady(Completion<Empty>) const;

private:
    std::shared_ptr<PasswordWalletRepository> repository_;
    std::shared_ptr<AsyncExecutor> executor_;
};
} // namespace passwall::server

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

#include "HealthService.h"
#include "AsyncExecutor.h"
#include "repositories/PasswordWalletRepository.h"

#include <stdexcept>

namespace passwall::server
{
HealthService::HealthService(std::shared_ptr<PasswordWalletRepository> repository,
                             std::shared_ptr<AsyncExecutor> executor)
    : repository_(std::move(repository)), executor_(std::move(executor))
{
    if (!repository_ || !executor_)
        throw std::invalid_argument("Health dependencies are required");
}

void HealthService::checkReady(Completion<Empty> done) const
{
    executor_->submit<Empty>(std::move(done),
                             [repository = repository_]
                             {
                                 repository->checkReady();
                                 return Result<Empty>::success();
                             });
}
} // namespace passwall::server

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

#pragma once

#include "ServiceTypes.h"
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

namespace passwall::server
{
// Shared by the service layer. Blocking work never runs on HTTP event loops.
class AsyncExecutor final
{
public:
    AsyncExecutor();
    ~AsyncExecutor();
    // Drain pending tasks before destroying services/dependencies at shutdown.
    void shutdown();

    template <typename T, typename F> void submit(Completion<T> done, F operation)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!stopping_ && jobs_.size() < 128)
            {
                jobs_.emplace(
                    [done, operation = std::move(operation)]() mutable
                    {
                        Result<T> result;
                        try
                        {
                            result = operation();
                        }
                        catch (...)
                        {
                            result = Result<T>::failure(ServiceError::internalError);
                        }
                        done(std::move(result));
                    });
                wake_.notify_one();
                return;
            }
        }
        done(Result<T>::failure(ServiceError::unavailable));
    }

private:
    void run();
    std::mutex mutex_;
    std::condition_variable wake_;
    std::queue<std::function<void()>> jobs_;
    bool stopping_{false};
    std::vector<std::thread> workers_;
};
} // namespace passwall::server

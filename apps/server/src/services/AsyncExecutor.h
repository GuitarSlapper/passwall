#pragma once

#include "ServiceTypes.h"
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

namespace passwall
{
// Shared by the service layer. Blocking work never runs on HTTP event loops.
class AsyncExecutor final
{
  public:
    AsyncExecutor();
    ~AsyncExecutor();
    // Drain pending tasks before destroying services/dependencies at shutdown.
    void shutdown();

    template <typename T, typename F>
    void submit(Completion<T> done, F operation)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!stopping_ && jobs_.size() < 128)
            {
                jobs_.emplace([done, operation = std::move(operation)]() mutable {
                    Result<T> result;
                    try { result = operation(); }
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
}  // namespace passwall

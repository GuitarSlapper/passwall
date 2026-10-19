#include "AsyncExecutor.h"

namespace passwall::server
{
AsyncExecutor::AsyncExecutor()
{
    try
    {
        for (int i = 0; i < 2; ++i)
            workers_.emplace_back([this] { run(); });
    }
    catch (...)
    {
        shutdown();
        throw;
    }
}

AsyncExecutor::~AsyncExecutor()
{
    shutdown();
}

void AsyncExecutor::shutdown()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    wake_.notify_all();
    for (auto& worker : workers_)
        if (worker.joinable())
            worker.join();
}

void AsyncExecutor::run()
{
    for (;;)
    {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            wake_.wait(lock, [this] { return stopping_ || !jobs_.empty(); });
            if (jobs_.empty())
                return;
            job = std::move(jobs_.front());
            jobs_.pop();
        }
        job();
    }
}
} // namespace passwall::server

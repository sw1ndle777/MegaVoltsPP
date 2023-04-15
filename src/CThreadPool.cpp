#include "CThreadPool.h"
namespace BaseLib
{
    CThreadPool::CThreadPool() {}
    CThreadPool::~CThreadPool()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        stop_ = true;
        condition_.notify_all();
        for (auto& worker : workers_) worker.join();
    }
    CThreadPool::CThreadPool(CThreadPool&& other) noexcept
    {
        workers_ = std::move(other.workers_);
        m_concurrentThreads = other.m_concurrentThreads;
        queue_ = std::move(other.queue_);
        stop_ = other.stop_;
    }
    CThreadPool& CThreadPool::operator=(CThreadPool&& other) noexcept
    {
        // Move the thread pool's state to this object
        workers_ = std::move(other.workers_);
        m_concurrentThreads = other.m_concurrentThreads;
        queue_ = std::move(other.queue_);
        stop_ = other.stop_;

        // Clear the state of the other object
        other.m_concurrentThreads = 0;
        other.stop_ = false;

        return *this;
    }
    void CThreadPool::Initialize(std::uint32_t threads_count)
    {
        if (!threads_count)
            m_concurrentThreads = std::jthread::hardware_concurrency();
        else
            m_concurrentThreads = threads_count;

        for (std::uint32_t i = 0; i < m_concurrentThreads; i++)
        {
            workers_.emplace_back([this]()
                {
                    while (true)
                    {
                        std::unique_lock<std::mutex> lock(mutex_);
                        condition_.wait(lock, [this]() { return !queue_.empty() || stop_; });

                        if (stop_ && queue_.empty()) return;

                        auto task = std::move(queue_.front());
                        queue_.pop();
                        lock.unlock();
                        task();
                    }
                });
        }
    }

    
    std::unique_ptr<CThreadPool> ThreadPool = std::make_unique<CThreadPool>();
}
#include "CThreadPool.h"
namespace BaseLib
{
    CThreadPool::CThreadPool() {}
    CThreadPool::~CThreadPool()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        stop_ = true;
        condition_.notify_all();
    }
    CThreadPool::CThreadPool(CThreadPool&& other) noexcept
    {
        workers_ = std::move(other.workers_);
        m_concurrentThreads = other.m_concurrentThreads;
        queue_ = std::move(other.queue_);
        stop_.store(other.stop_.load());
    }
    CThreadPool& CThreadPool::operator=(CThreadPool&& other) noexcept
    {
        if (this != &other) // Self-assignment check
        {
            workers_ = std::move(other.workers_);
            m_concurrentThreads = other.m_concurrentThreads;
            queue_ = std::move(other.queue_);
            stop_.store(other.stop_.load());  // Use atomic load and store

            other.m_concurrentThreads = 0;
            other.stop_.store(false);  // Reset the atomic flag in the moved-from object
        }
        return *this;
    }
    void CThreadPool::Initialize(std::uint32_t threads_count)
    {
        if (!workers_.empty()) return; // Already initialized
        m_concurrentThreads = threads_count ? threads_count : std::jthread::hardware_concurrency();


        for (std::uint32_t i = 0; i < m_concurrentThreads; i++)
        {
            workers_.emplace_back([this]()
            {
                while (true)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mutex_);
                        condition_.wait(lock, [this]() { return !queue_.empty() || stop_; });

                        if (stop_ && queue_.empty()) return;

                        task = std::move(queue_.front());
                        queue_.pop();
                    }
                    task();
                }
            });
        }
    }

    
    std::unique_ptr<CThreadPool> ThreadPool = std::make_unique<CThreadPool>();
}
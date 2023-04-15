#pragma once
#ifndef CTHREADPOOL_H
#define CTHREADPOOL_H
#include <iostream>
#include <thread>
#include <vector>
#include <future>
#include <functional>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
namespace BaseLib
{
    class CThreadPool
    {
    public:
        CThreadPool();
        ~CThreadPool();
        CThreadPool(CThreadPool&& other) noexcept;
        CThreadPool& operator=(CThreadPool&& other) noexcept;
        void Initialize(std::uint32_t threads_count = 1);
        template<typename F>
        inline auto post(F&& f) -> std::future<std::invoke_result_t<F>>
        {
            using return_type = std::invoke_result_t<F>;
            auto task = std::make_shared<std::packaged_task<return_type()>>(std::forward<F>(f));
            std::future<return_type> result = task->get_future();
            {
                std::unique_lock<std::mutex> lock(mutex_);
                queue_.emplace([task]() { (*task)(); });
            }
            condition_.notify_one();
            return result;
        }
    private:
        std::vector<std::jthread> workers_;
        std::uint32_t m_concurrentThreads = 1;
        std::queue<std::function<void()>> queue_;
        std::mutex mutex_;
        std::condition_variable condition_;
        bool stop_ = false;
    };
    extern std::unique_ptr<CThreadPool> ThreadPool;
}
#endif
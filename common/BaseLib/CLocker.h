#pragma once
#include <shared_mutex>
#include <utility>
#include <type_traits>

namespace BaseLib
{
    template<class TLock, class T>
    class CLocker
    {
    public:
        using lock_type = TLock;
        using resource_type = T;

        CLocker(TLock&& lk, T& res, bool is_null = false)
            : m_lock(std::move(lk)), m_ptr(&res), m_is_null(is_null) {
        }

        CLocker(const CLocker&) = delete;
        CLocker& operator=(const CLocker&) = delete;
        CLocker(CLocker&&) noexcept = default;
        CLocker& operator=(CLocker&&) noexcept = default;

        [[nodiscard]] T* operator->() noexcept { return m_ptr; }
        [[nodiscard]] const T* operator->() const noexcept { return m_ptr; }
        [[nodiscard]] T& operator*() noexcept { return *m_ptr; }
        [[nodiscard]] const T& operator*() const noexcept { return *m_ptr; }

        void unlock() { m_lock.unlock(); }
        void lock() { m_lock.lock(); }
        bool try_lock() { return m_lock.try_lock(); }
        [[nodiscard]] bool owns_lock() const noexcept { return m_lock.owns_lock(); }

        [[nodiscard]] explicit operator bool() const noexcept { return !m_is_null; }
        [[nodiscard]] bool is_null() const noexcept { return m_is_null; }

    private:
        TLock m_lock;
        T* m_ptr{ nullptr };
        bool  m_is_null{ false };
    };

    template<class TLock, class T>
    CLocker(TLock&&, T&, bool) -> CLocker<std::remove_cvref_t<TLock>, std::remove_reference_t<T>>;
}
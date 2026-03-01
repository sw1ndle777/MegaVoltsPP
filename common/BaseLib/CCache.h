#pragma once
#include <shared_mutex>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>
#include <concepts>
#include <boost_unordered.hpp>
#include "BaseLib/CLocker.h"

namespace BaseLib
{
    struct shared_t {}; struct unique_t {};
    inline constexpr shared_t shared{};
    inline constexpr unique_t unique{};

    template<class C>
    concept map_like = requires(C c, const typename C::key_type & k) {
        typename C::mapped_type;
        { c.find(k) } -> std::same_as<typename C::iterator>;
        { c.erase(k) } -> std::convertible_to<std::size_t>;
        { c.size() } -> std::convertible_to<std::size_t>;
    };

    template<class C>
    concept vector_like = requires(C c, typename C::value_type v) {
        typename C::value_type;
        { c.size() } -> std::convertible_to<std::size_t>;
        { c.clear() };
        { c.push_back(std::move(v)) };
    };

    template<class C>
    concept set_like = requires(C c, typename C::value_type v) {
        typename C::value_type;
        { c.size() } -> std::convertible_to<std::size_t>;
        { c.find(v) } -> std::same_as<typename C::iterator>;
        { c.erase(v) } -> std::convertible_to<std::size_t>;
        { c.insert(std::move(v)) };
    };

    template<typename T, typename = void>
    struct has_member_mutex : std::false_type {};
    template<typename T>
    struct has_member_mutex<T, std::void_t<decltype(std::declval<T&>().mutex)>> : std::true_type {};

    template<class Container, class Mutex = std::shared_mutex>
    class CCache
    {
    public:
        using container_type = Container;
        using mutex_type = Mutex;

        CCache() = default;
        CCache(const CCache&) = delete;
        CCache& operator=(const CCache&) = delete;

        CCache* operator->() noexcept { return this; }
        const CCache* operator->() const noexcept { return this; }

        [[nodiscard]] auto get_all(shared_t) { return CLocker{ std::shared_lock<mutex_type>(m_mtx), m_cont }; }
        [[nodiscard]] auto get_all(unique_t) { return CLocker{ std::unique_lock<mutex_type>(m_mtx), m_cont }; }

        void clear()
        {
            std::unique_lock<mutex_type> l(m_mtx);
            m_cont.clear();
        }

        std::size_t size() const
        {
            std::shared_lock<mutex_type> l(m_mtx);
            return m_cont.size();
        }
        template<class K>
            requires (map_like<container_type>&& vector_like<typename container_type::mapped_type>)
        std::size_t size(const K& k)
        {
            std::shared_lock<mutex_type> l(m_mtx);
            auto it = m_cont.find(k);
            return (it != m_cont.end()) ? it->second.size() : 0;
        }

        template<class K1>
            requires (map_like<container_type>&& map_like<typename container_type::mapped_type>)
        std::size_t size(const K1& k1)
        {
            std::shared_lock<mutex_type> l(m_mtx);
            auto it = m_cont.find(k1);
            return (it != m_cont.end()) ? it->second.size() : 0;
        }

        // CONTAINS

        template<class V>
            requires (set_like<container_type>)
        bool contains(const V& v) const
        {
            std::shared_lock<mutex_type> l(m_mtx);
            return m_cont.find(v) != m_cont.end();
        }
        template<class K>
            requires (map_like<container_type>)
        bool contains(const K& k) const
        {
            std::shared_lock<mutex_type> l(m_mtx);
            return m_cont.find(k) != m_cont.end();
        }
        template<class K1, class K2>
            requires (map_like<container_type>&& map_like<typename container_type::mapped_type>)
        bool contains(const K1& k1, const K2& k2) const
        {
            std::shared_lock<mutex_type> l(m_mtx);
            auto it = m_cont.find(k1);
            if (it == m_cont.end()) return false;
            return it->second.find(k2) != it->second.end();
        }
        template<class T>
            requires (vector_like<container_type>)
        bool contains_value(const T& value) const
        {
            std::shared_lock<mutex_type> ml(m_mtx);
            return std::find(m_cont.begin(), m_cont.end(), value) != m_cont.end();
        }

        template<class V>
            requires (set_like<container_type>)
        bool insert(V&& v)
        {
            std::unique_lock<mutex_type> l(m_mtx);
            if constexpr (requires(container_type & c, V && x) { c.emplace(std::forward<V>(x)); })
                return m_cont.emplace(std::forward<V>(v)).second;
            else
                return m_cont.insert(std::forward<V>(v)).second;
        }
        template<class K, class V>
            requires (map_like<container_type>)
        bool insert(const K& k, V&& v)
        {
            std::unique_lock<mutex_type> l(m_mtx);
            return m_cont.emplace(k, std::forward<V>(v)).second;
        }

        template<class K1, class K2, class V>
            requires (map_like<container_type>&& map_like<typename container_type::mapped_type>)
        bool insert(const K1& k1, const K2& k2, V&& v)
        {
            std::unique_lock<mutex_type> l(m_mtx);
            auto& inner = m_cont[k1];
            return inner.emplace(k2, std::forward<V>(v)).second;
        }

        template<class V>
            requires (set_like<container_type>)
        void emplace_back(V&& v)
        {
            std::unique_lock<mutex_type> l(m_mtx);
            (void)m_cont.insert(std::forward<V>(v));
        }
        template<class V>
            requires (set_like<container_type>)
        void push_back(V&& v)
        {
            emplace_back(std::forward<V>(v));
        }

        // -------- map → vector sugar --------
        template<class K, class V>
            requires (map_like<container_type>&& vector_like<typename container_type::mapped_type>)
        void emplace_back(const K& k, V&& v)
        {
            std::unique_lock<mutex_type> l(m_mtx);
            m_cont[k].push_back(std::forward<V>(v));
        }
        template<class K1, class K2, class V>
            requires (map_like<container_type>&&
        map_like<typename container_type::mapped_type>&&
            vector_like<typename container_type::mapped_type::mapped_type>)
            void emplace_back(const K1& k1, const K2& k2, V&& v)
        {
            std::unique_lock<mutex_type> l(m_mtx);
            m_cont[k1][k2].push_back(std::forward<V>(v));
        }
        template<class T>
            requires (vector_like<container_type>)
        void emplace_back(T&& v)
        {
            std::unique_lock<mutex_type> l(m_mtx);
            m_cont.push_back(std::forward<T>(v));
        }

        template<class K, class T, class Eq, class Updater>
            requires (map_like<container_type>&& vector_like<typename container_type::mapped_type>)
        bool upsert_in_vector(const K& k, const T& val, Eq&& equal, Updater&& upd)
        {
            std::unique_lock<mutex_type> ml(m_mtx);
            auto it = m_cont.find(k);
            if (it == m_cont.end()) {
                auto& vec = m_cont[k];
                vec.push_back(val);
                return true;
            }

            auto& vec = it->second;
            if (auto it2 = std::find_if(vec.begin(), vec.end(),
                [&](const auto& cur) { return std::invoke(equal, cur, val); });
                it2 != vec.end()) {
                std::invoke(std::forward<Updater>(upd), *it2, val);
                return false;
            }
            vec.push_back(val);
            return true;
        }
        template<class K, class T, class Eq>
            requires (map_like<container_type>&& vector_like<typename container_type::mapped_type>)
        bool upsert_in_vector_assign(const K& k, const T& val, Eq&& equal)
        {
            return upsert_in_vector(k, val, std::forward<Eq>(equal),
                [](auto& dst, const auto& src) { dst = src; });
        }


        template<class K, class Updater, class... Args>
            requires (map_like<container_type>)
        [[nodiscard]] auto upsert_in_map(const K& k, Updater&& updater, Args&&... emplace_args)
        {
            using M = typename container_type::mapped_type;

            if constexpr (has_member_mutex<M>::value)
            {
                std::unique_lock<mutex_type> ml(m_mtx);

                auto it = m_cont.find(k);
                if (it != m_cont.end())
                {
                    auto el = std::unique_lock<std::shared_mutex>(it->second.mutex);
                    ml.unlock();

                    std::invoke(std::forward<Updater>(updater), it->second);
                    return CLocker{ std::move(el), it->second };
                }
                auto [insIt, inserted] = m_cont.try_emplace(k, std::forward<Args>(emplace_args)...);
                auto el = std::unique_lock<std::shared_mutex>(insIt->second.mutex);
                ml.unlock();
                std::invoke(std::forward<Updater>(updater), insIt->second);

                return CLocker{ std::move(el), insIt->second };
            }
            else
            {
                std::unique_lock<mutex_type> ml(m_mtx);
                auto it = m_cont.find(k);
                if (it != m_cont.end())
                {
                    std::invoke(std::forward<Updater>(updater), it->second);
                    return CLocker{ std::move(ml), it->second };
                }

                auto [insIt, inserted] = m_cont.try_emplace(k, std::forward<Args>(emplace_args)...);
                std::invoke(std::forward<Updater>(updater), insIt->second);

                return CLocker{ std::move(ml), insIt->second };
            }
        }

        template<class K, class V>
            requires (map_like<container_type>)
        [[nodiscard]] auto upsert_in_map_assign(const K& k, V&& v)
        {
            return update_or_emplace(
                k,
                [&](auto& mapped) { mapped = std::forward<V>(v); },
                std::forward<V>(v)
            );
        }


        template<class V>
            requires (set_like<container_type>)
        bool erase(const V& v)
        {
            std::unique_lock<mutex_type> l(m_mtx);
            return m_cont.erase(v) > 0;
        }


        template<class K>
            requires (map_like<container_type>)
        bool erase(const K& k)
        {
            std::unique_lock<mutex_type> l(m_mtx);
            return m_cont.erase(k) > 0;
        }


        template<class K1, class K2>
            requires (map_like<container_type>&& map_like<typename container_type::mapped_type>)
        bool erase(const K1& k1, const K2& k2)
        {
            std::unique_lock<mutex_type> l(m_mtx);
            auto it = m_cont.find(k1);
            if (it == m_cont.end()) return false;
            return it->second.erase(k2) > 0;
        }



        template<class T>
            requires (vector_like<container_type>)
        bool erase_value(const T& value)
        {
            std::unique_lock<mutex_type> ml(m_mtx);
            auto& vec = m_cont;
            auto it = std::find(vec.begin(), vec.end(), value);
            if (it == vec.end()) return false;
            vec.erase(it);
            return true;
        }

        template<class Pred>
            requires (vector_like<container_type>)
        std::size_t erase_if(Pred&& pred)
        {
            std::unique_lock<mutex_type> ml(m_mtx);
            auto& vec = m_cont;
            auto before = vec.size();
            vec.erase(std::remove_if(vec.begin(), vec.end(), std::forward<Pred>(pred)), vec.end());
            return before - vec.size();
        }

        template<class T>
            requires (vector_like<container_type>)
        bool erase_one_unordered(const T& value)
        {
            std::unique_lock<mutex_type> ml(m_mtx);
            auto& vec = m_cont;
            auto it = std::find(vec.begin(), vec.end(), value);
            if (it == vec.end()) return false;
            *it = std::move(vec.back());
            vec.pop_back();
            return true;
        }

        template<class Pred>
            requires (vector_like<container_type>)
        std::size_t erase_all_unordered_if(Pred&& pred)
        {
            std::unique_lock<mutex_type> ml(m_mtx);
            auto& vec = m_cont;
            std::size_t removed = 0;
            for (std::size_t i = 0; i < vec.size();)
            {
                if (std::invoke(pred, vec[i]))
                {
                    vec[i] = std::move(vec.back());
                    vec.pop_back();
                    removed++;
                }
                else i++;
            }
            return removed;
        }
        template<class K, class V>
            requires (map_like<container_type>&&
        vector_like<typename container_type::mapped_type>)
            bool erase_value(const K& k, const V& value)
        {
            std::unique_lock<mutex_type> ml(m_mtx);
            auto it = m_cont.find(k);
            if (it == m_cont.end()) return false;

            auto& vec = it->second;
            auto old = vec.size();
            vec.erase(std::remove(vec.begin(), vec.end(), value), vec.end());
            return vec.size() != old;
        }

        template<class K, class Pred>
            requires (map_like<container_type>&&
        vector_like<typename container_type::mapped_type>)
            std::size_t erase_if(const K& k, Pred&& pred)
        {
            std::unique_lock<mutex_type> ml(m_mtx);
            auto it = m_cont.find(k);
            if (it == m_cont.end()) return 0;

            auto& vec = it->second;
            auto before = vec.size();
            vec.erase(std::remove_if(vec.begin(), vec.end(), std::forward<Pred>(pred)), vec.end());
            return before - vec.size();
        }

        template<class K>
            requires (map_like<container_type>&&
        vector_like<typename container_type::mapped_type>)
            bool erase_key_if_empty(const K& k)
        {
            std::unique_lock<mutex_type> ml(m_mtx);
            auto it = m_cont.find(k);
            if (it == m_cont.end()) return false;
            if (it->second.empty()) { m_cont.erase(it); return true; }
            return false;
        }

        template<class LockTag, class K>
            requires (map_like<container_type>)
        [[nodiscard]] auto get(const K& k)
        {
            using M = typename container_type::mapped_type;

            if constexpr (has_member_mutex<M>::value)
            {
                std::shared_lock<mutex_type> ml(m_mtx);
                auto it = m_cont.find(k);
                if (it != m_cont.end())
                {
                    if constexpr (std::is_same_v<LockTag, shared_t>)
                    {
                        auto el = std::shared_lock<std::shared_mutex>(it->second.mutex);
                        ml.unlock();
                        return CLocker{ std::move(el), it->second };
                    }
                    else
                    {
                        auto el = std::unique_lock<std::shared_mutex>(it->second.mutex);
                        ml.unlock();
                        return CLocker{ std::move(el), it->second };
                    }
                }
                ml.unlock();
                static thread_local M null_entry{};
                if constexpr (std::is_same_v<LockTag, shared_t>)
                    return CLocker{ std::shared_lock<std::shared_mutex>(null_entry.mutex, std::defer_lock), null_entry, true };
                else
                    return CLocker{ std::unique_lock<std::shared_mutex>(null_entry.mutex, std::defer_lock), null_entry, true };
            }
            else
            {
                if constexpr (std::is_same_v<LockTag, shared_t>)
                {
                    std::shared_lock<mutex_type> ml(m_mtx);
                    auto it = m_cont.find(k);
                    if (it != m_cont.end()) return CLocker{ std::move(ml), it->second };
                    static thread_local mutex_type null_mtx;
                    static thread_local M null_val{};
                    return CLocker{ std::shared_lock<mutex_type>(null_mtx, std::defer_lock), null_val, true };
                }
                else
                {
                    std::unique_lock<mutex_type> ml(m_mtx);
                    auto it = m_cont.find(k);
                    if (it != m_cont.end()) return CLocker{ std::move(ml), it->second };
                    static thread_local mutex_type null_mtx;
                    static thread_local M null_val{};
                    return CLocker{ std::unique_lock<mutex_type>(null_mtx, std::defer_lock), null_val, true };
                }
            }
        }

        template<class K, class... Args>
            requires (map_like<container_type>)
        [[nodiscard]] auto get_or_emplace(const K& k, Args&&... args)
        {
            using M = typename container_type::mapped_type;
            std::unique_lock<mutex_type> ml(m_mtx);
            auto [it, ins] = m_cont.try_emplace(k, std::forward<Args>(args)...);
            if constexpr (has_member_mutex<M>::value)
            {
                auto el = std::unique_lock<std::shared_mutex>(it->second.mutex);
                ml.unlock();
                return CLocker{ std::move(el), it->second };
            }
            else
            {
                return CLocker{ std::move(ml), it->second };
            }
        }

        template<class LockTag, class K1, class K2>
            requires (map_like<container_type>&& map_like<typename container_type::mapped_type>)
        [[nodiscard]] auto get(const K1& k1, const K2& k2)
        {
            using InnerMap = typename container_type::mapped_type;
            using InnerMapped = typename InnerMap::mapped_type;

            if constexpr (has_member_mutex<InnerMapped>::value)
            {
                std::shared_lock<mutex_type> ml(m_mtx);
                auto o = m_cont.find(k1);
                if (o != m_cont.end())
                {
                    auto i = o->second.find(k2);
                    if (i != o->second.end())
                    {
                        if constexpr (std::is_same_v<LockTag, shared_t>)
                        {
                            auto el = std::shared_lock<std::shared_mutex>(i->second.mutex);
                            ml.unlock();
                            return CLocker{ std::move(el), i->second };
                        }
                        else
                        {
                            auto el = std::unique_lock<std::shared_mutex>(i->second.mutex);
                            ml.unlock();
                            return CLocker{ std::move(el), i->second };
                        }
                    }
                }
                static thread_local InnerMapped null_entry{};
                if constexpr (std::is_same_v<LockTag, shared_t>)
                    return CLocker{ std::shared_lock<std::shared_mutex>(null_entry.mutex, std::defer_lock), null_entry, true };
                else
                    return CLocker{ std::unique_lock<std::shared_mutex>(null_entry.mutex, std::defer_lock), null_entry, true };
            }
            else
            {
                if constexpr (std::is_same_v<LockTag, shared_t>)
                {
                    std::shared_lock<mutex_type> ml(m_mtx);
                    auto o = m_cont.find(k1);
                    if (o != m_cont.end())
                    {
                        auto i = o->second.find(k2);
                        if (i != o->second.end())
                            return CLocker{ std::move(ml), i->second };
                    }
                    static thread_local mutex_type null_mtx;
                    static thread_local InnerMapped null_val{};
                    return CLocker{ std::shared_lock<mutex_type>(null_mtx, std::defer_lock), null_val, true };
                }
                else
                {
                    std::unique_lock<mutex_type> ml(m_mtx);
                    auto o = m_cont.find(k1);
                    if (o != m_cont.end())
                    {
                        auto i = o->second.find(k2);
                        if (i != o->second.end())
                            return CLocker{ std::move(ml), i->second };
                    }
                    static thread_local mutex_type null_mtx;
                    static thread_local InnerMapped null_val{};
                    return CLocker{ std::unique_lock<mutex_type>(null_mtx, std::defer_lock), null_val, true };
                }
            }
        }

        template<class LockTag, class Pred>
            requires (map_like<container_type>)
        [[nodiscard]] auto get_by_filter(Pred&& pred)
        {
            using M = typename container_type::mapped_type;

            if constexpr (has_member_mutex<M>::value)
            {
                std::shared_lock<mutex_type> ml(m_mtx);
                for (auto it = m_cont.begin(); it != m_cont.end(); ++it)
                {
                    if (std::invoke(pred, it->first, it->second))
                    {
                        if constexpr (std::is_same_v<LockTag, shared_t>)
                        {
                            auto el = std::shared_lock<std::shared_mutex>(it->second.mutex);
                            ml.unlock();
                            return CLocker{ std::move(el), it->second };
                        }
                        else
                        {
                            auto el = std::unique_lock<std::shared_mutex>(it->second.mutex);
                            ml.unlock();
                            return CLocker{ std::move(el), it->second };
                        }
                    }
                }
                static thread_local M null_entry{};
                if constexpr (std::is_same_v<LockTag, shared_t>)
                    return CLocker{ std::shared_lock<std::shared_mutex>(null_entry.mutex, std::defer_lock), null_entry, true };
                else
                    return CLocker{ std::unique_lock<std::shared_mutex>(null_entry.mutex, std::defer_lock), null_entry, true };
            }
            else
            {
                if constexpr (std::is_same_v<LockTag, shared_t>)
                {
                    std::shared_lock<mutex_type> ml(m_mtx);
                    for (auto it = m_cont.begin(); it != m_cont.end(); ++it)
                    {
                        if (std::invoke(pred, it->first, it->second))
                        {
                            return CLocker{ std::move(ml), it->second };
                        }
                    }
                    static thread_local mutex_type null_mtx;
                    static thread_local M null_val{};
                    return CLocker{ std::shared_lock<mutex_type>(null_mtx, std::defer_lock), null_val, true };
                }
                else
                {
                    std::unique_lock<mutex_type> ml(m_mtx);
                    for (auto it = m_cont.begin(); it != m_cont.end(); ++it)
                    {
                        if (std::invoke(pred, it->first, it->second))
                        {
                            return CLocker{ std::move(ml), it->second };
                        }
                    }
                    static thread_local mutex_type null_mtx;
                    static thread_local M null_val{};
                    return CLocker{ std::unique_lock<mutex_type>(null_mtx, std::defer_lock), null_val, true };
                }
            }
        }

        

        container_type& unsafe_ref() noexcept { return m_cont; }
        const container_type& unsafe_ref() const noexcept { return m_cont; }
        mutex_type& mutex() noexcept { return m_mtx; }

    private:
        mutable mutex_type m_mtx;
        container_type     m_cont;
    };
}
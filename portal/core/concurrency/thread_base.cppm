//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#include "portal/core/profile_macros.h"

export module portal.core.concurrency:thread;

import std.compat;

export namespace portal
{

enum class ThreadPriority : uint8_t
{
    Low,
    Default,
    High
};

enum class ThreadAffinity : uint8_t
{
    Default,
    Core,
    CoreLean
};

struct ThreadProperties
{
    std::string name;
    ThreadPriority priority = ThreadPriority::Default;
    ThreadAffinity affinity = ThreadAffinity::Default;
    uint16_t core = 0;
};

/**
 * A base thread object, with a similar api to `std::jthread`
 */
class ThreadBase
{
public:
    ThreadBase() = default;
    explicit ThreadBase(ThreadProperties properties) : properties(std::move(properties)) {}

    virtual ~ThreadBase() { try_cancel_and_join(); }

    ThreadBase(const ThreadBase&) = delete;
    ThreadBase(ThreadBase&&) noexcept = default;

    ThreadBase& operator=(const ThreadBase&) = delete;
    ThreadBase& operator=(ThreadBase&& other) noexcept
    {
        if (this == &other)
            return *this;

        try_cancel_and_join();
        thread = std::move(other.thread);
        properties = std::move(other.properties);
        return *this;
    }

    [[nodiscard]] bool joinable() const noexcept { return thread.joinable(); }

    void join() { thread.join(); }

    void detach() { thread.detach(); }

    [[nodiscard]] std::thread::id get_id() const noexcept { return thread.get_id(); }

    [[nodiscard]] std::string_view get_name() const noexcept { return properties.name; }

    bool request_stop() noexcept { return thread.request_stop(); }

protected:
    // NOLINTBEGIN(cppcoreguidelines-missing-std-forward, readability-redundant-parentheses)
    template<typename F, typename... Args>
    static auto make_callable(F&& f, Args&&... args)
    {
        if constexpr (std::is_invocable_v<std::decay_t<F>, std::stop_token, Args...>)
        {
            return [func = std::decay_t<F>(std::forward<F>(f)),
                    ... bound = std::tuple<Args&&>(std::forward<Args>(args))](std::stop_token st) mutable
            {
                return std::apply(
                        [&](auto&... t) { return std::invoke(func, st, (std::get<0>(t))...); },
                        std::forward_as_tuple(bound...)
                );
            };
        }
        else
        {

            return [func = std::decay_t<F>(std::forward<F>(f)), // NOLINTNEXTLINE(performance-unnecessary-value-param)
                    ... bound = std::tuple<Args&&>(std::forward<Args>(args))](std::stop_token) mutable
            {
                return std::apply(
                        [&](auto&... t) { return std::invoke(func, (std::get<0>(t))...); },
                        std::forward_as_tuple(bound...)
                );
            };
        }
    }
    // NOLINTEND(cppcoreguidelines-missing-std-forward, readability-redundant-parentheses)

    void try_cancel_and_join()
    {
        if (thread.joinable())
        {
            thread.request_stop();
            thread.join();
        }
    }

    static void set_name([[maybe_unused]] const std::string& name) { PORTAL_NAME_THREAD(name.c_str()); }

protected:
    ThreadProperties properties;
    std::jthread thread;
};

} // namespace portal

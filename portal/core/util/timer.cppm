//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#include "portal/core/log_macros.h"

export module portal.core.util:timer;

import std;
import portal.core.log;

export namespace portal
{

namespace timing
{
    constexpr auto MILLI_TO_SECOND = 1000u;
    constexpr auto MICRO_TO_SECOND = 1000000u;
    constexpr auto NANO_TO_SECOND = 1000000000u;

    using Seconds = std::ratio<1>;
    using Milliseconds = std::ratio<1, MILLI_TO_SECOND>;
    using Microseconds = std::ratio<1, MICRO_TO_SECOND>;
    using Nanoseconds = std::ratio<1, NANO_TO_SECOND>;
} // namespace timing

/**
 * @brief Encapsulates basic usage of chrono, providing a means to calculate float
 *        durations between time points via function calls.
 */
template<typename DefaultResolution = timing::Seconds, typename Clock = std::chrono::steady_clock>
class Timer
{
public:
    Timer() : start_time{Clock::now()}, previous_tick{Clock::now()} {}

    /**
     * @brief Starts the timer, elapsed() now returns the duration since start()
     */
    void start()
    {
        if (!running)
        {
            running = true;
            start_time = Clock::now();
        }
    }

    /**
     * @brief Laps the timer, elapsed() now returns the duration since the last lap()
     */
    void lap()
    {
        lapping = true;
        lap_time = Clock::now();
    }

    /**
     * @brief Stops the timer, elapsed() now returns 0
     * @return The total execution time between `start()` and `stop()`
     */
    template<typename Resolution = DefaultResolution>
    float stop()
    {
        if (!running)
            return 0;

        running = false;
        lapping = false;
        auto duration = std::chrono::duration<float, Resolution>(Clock::now() - start_time);
        start_time = Clock::now();
        lap_time = Clock::now();

        return duration.count();
    }

    /**
     * @brief Calculates the time difference between now and when the timer was started
     *        if lap() was called, then between now and when the timer was last lapped
     * @return The duration between the two time points (default in seconds)
     */
    template<typename Resolution = DefaultResolution>
    float elapsed()
    {
        if (!running)
            return 0;

        typename Clock::time_point start = start_time;
        if (lapping)
            start = lap_time;

        return std::chrono::duration<float, Resolution>(Clock::now() - start).count();
    }

    /**
     * @brief Calculates the time difference between now and the last time this function was called
     * @return The duration between the two time points (default in seconds)
     */
    template<typename Resolution = DefaultResolution>
    float tick()
    {
        const auto now = Clock::now();
        auto duration = std::chrono::duration<float, Resolution>(now - previous_tick);
        previous_tick = now;
        return duration.count();
    }

    [[nodiscard]] bool is_running() const { return running; }


private:
    bool running{false};
    bool lapping{false};
    Clock::time_point start_time;
    Clock::time_point lap_time;
    Clock::time_point previous_tick;
};

namespace timing
{
    template<typename Resolution>
    constexpr std::string_view unit_suffix()
    {
        if constexpr (std::is_same_v<Resolution, Seconds>)
            return "s";
        else if constexpr (std::is_same_v<Resolution, Milliseconds>)
            return "ms";
        else if constexpr (std::is_same_v<Resolution, Microseconds>)
            return "us";
        else if constexpr (std::is_same_v<Resolution, Nanoseconds>)
            return "ns";
        else
            return "units";
    }
} // namespace timing

template <typename Resolution = timing::Seconds, typename Clock = std::chrono::steady_clock>
class ScopedTimer
{
public:
    explicit ScopedTimer(const std::string_view& name) :
        name(name)
    {
        timer.start();
    }

    ~ScopedTimer() noexcept
    {
        auto duration = timer.stop();
        LOG_INFO_NAMED("Timer", "\"{}\" - {} {}", name, duration, timing::unit_suffix<Resolution>());
    }

private:
    std::string_view name;
    Timer<Resolution, Clock> timer;
};

} // namespace portal

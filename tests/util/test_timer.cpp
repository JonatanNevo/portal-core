//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <catch2/catch_all.hpp>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

import portal.core;
import std;

using Catch::Matchers::WithinAbs;

namespace
{

/**
 * Manually driven clock, time only moves when `advance()` is called.
 */
struct MockClock
{
    using duration = std::chrono::nanoseconds;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<MockClock>;
    static constexpr bool is_steady = true;

    static time_point now() noexcept { return current; }

    static void advance(const duration d) { current += d; }
    static void reset() { current = time_point{}; }

    static inline time_point current{};
};

using MockTimer = portal::Timer<portal::timing::Seconds, MockClock>;

} // namespace

SCENARIO("A timer measures elapsed time between start and stop", "[unit][core][util]")
{
    using namespace std::chrono_literals;
    MockClock::reset();

    GIVEN("A newly created timer")
    {
        MockTimer timer;

        THEN("it is not running")
        {
            REQUIRE_FALSE(timer.is_running());
        }

        THEN("elapsed returns 0")
        {
            MockClock::advance(5s);
            REQUIRE(timer.elapsed() == 0.0f);
        }

        THEN("stop returns 0")
        {
            MockClock::advance(5s);
            REQUIRE(timer.stop() == 0.0f);
        }

        WHEN("it is started")
        {
            timer.start();

            THEN("it is running")
            {
                REQUIRE(timer.is_running());
            }

            THEN("elapsed is 0 before the clock advances")
            {
                REQUIRE(timer.elapsed() == 0.0f);
            }

            AND_WHEN("the clock advances")
            {
                MockClock::advance(1500ms);

                THEN("elapsed returns the time since start")
                {
                    REQUIRE_THAT(timer.elapsed(), WithinAbs(1.5, 1e-6));
                }

                THEN("elapsed honours the requested resolution")
                {
                    REQUIRE_THAT(timer.elapsed<portal::timing::Milliseconds>(), WithinAbs(1500.0, 1e-3));
                    REQUIRE_THAT(timer.elapsed<portal::timing::Microseconds>(), WithinAbs(1500000.0, 1e-1));
                }

                THEN("stop returns the total duration and stops the timer")
                {
                    REQUIRE_THAT(timer.stop(), WithinAbs(1.5, 1e-6));
                    REQUIRE_FALSE(timer.is_running());
                    REQUIRE(timer.elapsed() == 0.0f);
                }

                THEN("stop honours the requested resolution")
                {
                    REQUIRE_THAT(timer.stop<portal::timing::Milliseconds>(), WithinAbs(1500.0, 1e-3));
                }

                THEN("a second stop returns 0")
                {
                    timer.stop();
                    MockClock::advance(1s);
                    REQUIRE(timer.stop() == 0.0f);
                }
            }

            AND_WHEN("start is called again while running")
            {
                MockClock::advance(2s);
                timer.start();
                MockClock::advance(1s);

                THEN("the original start time is kept")
                {
                    REQUIRE_THAT(timer.elapsed(), WithinAbs(3.0, 1e-6));
                }
            }

            AND_WHEN("it is stopped and restarted")
            {
                MockClock::advance(2s);
                timer.stop();
                MockClock::advance(10s);
                timer.start();
                MockClock::advance(1s);

                THEN("elapsed only counts time since the restart")
                {
                    REQUIRE_THAT(timer.elapsed(), WithinAbs(1.0, 1e-6));
                }
            }
        }
    }

    GIVEN("A timer with a millisecond default resolution")
    {
        portal::Timer<portal::timing::Milliseconds, MockClock> timer;
        timer.start();
        MockClock::advance(250ms);

        THEN("elapsed and stop report milliseconds by default")
        {
            REQUIRE_THAT(timer.elapsed(), WithinAbs(250.0, 1e-3));
            REQUIRE_THAT(timer.stop(), WithinAbs(250.0, 1e-3));
        }
    }
}

SCENARIO("A timer can be lapped", "[unit][core][util]")
{
    using namespace std::chrono_literals;
    MockClock::reset();

    GIVEN("A running timer")
    {
        MockTimer timer;
        timer.start();
        MockClock::advance(2s);

        WHEN("it is lapped")
        {
            timer.lap();
            MockClock::advance(500ms);

            THEN("elapsed returns the time since the lap")
            {
                REQUIRE_THAT(timer.elapsed(), WithinAbs(0.5, 1e-6));
            }

            THEN("stop still returns the total time since start")
            {
                REQUIRE_THAT(timer.stop(), WithinAbs(2.5, 1e-6));
            }

            AND_WHEN("it is lapped again")
            {
                timer.lap();
                MockClock::advance(250ms);

                THEN("elapsed returns the time since the latest lap")
                {
                    REQUIRE_THAT(timer.elapsed(), WithinAbs(0.25, 1e-6));
                }
            }

            AND_WHEN("it is stopped and restarted")
            {
                timer.stop();
                MockClock::advance(1s);
                timer.start();
                MockClock::advance(3s);

                THEN("lapping is reset and elapsed counts from the restart")
                {
                    REQUIRE_THAT(timer.elapsed(), WithinAbs(3.0, 1e-6));
                }
            }
        }
    }
}

SCENARIO("A timer can tick", "[unit][core][util]")
{
    using namespace std::chrono_literals;
    MockClock::reset();

    GIVEN("A newly created timer")
    {
        MockTimer timer;

        THEN("the first tick measures from construction")
        {
            MockClock::advance(1s);
            REQUIRE_THAT(timer.tick(), WithinAbs(1.0, 1e-6));
        }

        THEN("tick works without the timer being started")
        {
            REQUIRE_FALSE(timer.is_running());
            MockClock::advance(100ms);
            REQUIRE_THAT(timer.tick(), WithinAbs(0.1, 1e-6));
        }

        WHEN("tick is called repeatedly")
        {
            MockClock::advance(100ms);
            timer.tick();
            MockClock::advance(16ms);

            THEN("each tick measures from the previous one")
            {
                REQUIRE_THAT(timer.tick<portal::timing::Milliseconds>(), WithinAbs(16.0, 1e-3));
                MockClock::advance(33ms);
                REQUIRE_THAT(timer.tick<portal::timing::Milliseconds>(), WithinAbs(33.0, 1e-3));
            }

            THEN("a tick with no time passed returns 0")
            {
                timer.tick();
                REQUIRE(timer.tick() == 0.0f);
            }
        }

        WHEN("the timer is started, lapped and stopped")
        {
            MockClock::advance(1s);
            timer.start();
            timer.lap();
            MockClock::advance(1s);
            timer.stop();

            THEN("tick is unaffected")
            {
                REQUIRE_THAT(timer.tick(), WithinAbs(2.0, 1e-6));
            }
        }
    }
}

SCENARIO("A scoped timer logs its lifetime on destruction", "[unit][core][util]")
{
    using namespace std::chrono_literals;
    MockClock::reset();

    GIVEN("The \"Timer\" logger writes to a string stream")
    {
        std::ostringstream stream;
        const auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(stream);
        sink->set_pattern("%v");
        spdlog::drop("Timer");
        spdlog::register_logger(std::make_shared<portal::log::Logger>("Timer", sink));

        WHEN("a scoped timer with second resolution goes out of scope")
        {
            {
                portal::ScopedTimer<portal::timing::Seconds, MockClock> scoped("work");
                MockClock::advance(2s);
            }

            THEN("the logged duration covers the scope, in seconds")
            {
                REQUIRE(stream.str() == "\"work\" - 2 s\n");
            }
        }

        WHEN("a scoped timer with millisecond resolution goes out of scope")
        {
            {
                portal::ScopedTimer<portal::timing::Milliseconds, MockClock> scoped("frame");
                MockClock::advance(16ms);
            }

            THEN("the logged duration is in milliseconds")
            {
                REQUIRE(stream.str() == "\"frame\" - 16 ms\n");
            }
        }

        spdlog::drop("Timer");
    }
}

SCENARIO("A timer works with the real steady clock", "[unit][core][util]")
{
    using namespace std::chrono_literals;

    GIVEN("A started timer using the default clock")
    {
        portal::Timer<portal::timing::Milliseconds> timer;
        timer.start();

        WHEN("some time passes")
        {
            std::this_thread::sleep_for(10ms);

            THEN("the measured duration is at least the slept duration")
            {
                REQUIRE(timer.elapsed() >= 10.0f);
                REQUIRE(timer.stop() >= 10.0f);
            }
        }
    }
}
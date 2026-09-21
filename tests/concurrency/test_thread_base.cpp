//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <catch2/catch_all.hpp>

import portal.core;
import portal.core.concurrency.platform;
import std;

SCENARIO("A default-constructed ThreadBase has no underlying thread", "[unit][core][concurrency][thread_base]")
{
    GIVEN("a default-constructed ThreadBase")
    {
        portal::ThreadBase thread;

        THEN("it is not joinable")
        {
            REQUIRE_FALSE(thread.joinable());
        }

        AND_THEN("it has no name")
        {
            REQUIRE(thread.get_name().empty());
        }

        AND_THEN("request_stop reports there was nothing to stop")
        {
            REQUIRE_FALSE(thread.request_stop());
        }
    }
}

SCENARIO("A ThreadBase constructed with properties exposes those properties", "[unit][core][concurrency][thread_base]")
{
    GIVEN("a ThreadBase constructed with a named ThreadProperties")
    {
        portal::ThreadBase thread{portal::ThreadProperties{.name = "worker"}};

        THEN("get_name returns the configured name")
        {
            REQUIRE(thread.get_name() == "worker");
        }

        AND_THEN("it is still not joinable, since no platform thread was started")
        {
            REQUIRE_FALSE(thread.joinable());
        }
    }
}

SCENARIO("portal::Thread runs the provided callable on its own thread", "[unit][core][concurrency][thread_base]")
{
    GIVEN("a Thread constructed with a callable that increments a counter")
    {
        std::atomic<int> counter{0};
        std::atomic<std::thread::id> ran_on{};

        portal::Thread thread(
                portal::ThreadProperties{.name = "counter-thread"},
                [&]
                {
                    ran_on.store(std::this_thread::get_id(), std::memory_order_relaxed);
                    counter.fetch_add(1, std::memory_order_relaxed);
                }
        );

        THEN("the thread is joinable and runs the callable exactly once, off the calling thread")
        {
            REQUIRE(thread.joinable());
            thread.join();

            REQUIRE(counter.load(std::memory_order_relaxed) == 1);
            REQUIRE(ran_on.load(std::memory_order_relaxed) != std::this_thread::get_id());
        }
    }
}

SCENARIO(
        "portal::Thread forwards extra constructor arguments to the callable",
        "[unit][core][concurrency][thread_base]"
)
{
    GIVEN("a Thread constructed with a callable that takes extra arguments")
    {
        std::atomic<int> result{0};

        portal::Thread thread(
                portal::ThreadProperties{.name = "sum-thread"},
                [&](const int a, const int b) { result.store(a + b, std::memory_order_relaxed); },
                20,
                22
        );

        THEN("the arguments are forwarded to the callable")
        {
            thread.join();
            REQUIRE(result.load(std::memory_order_relaxed) == 42);
        }
    }
}

SCENARIO("portal::Thread supports callables that observe a std::stop_token", "[unit][core][concurrency][thread_base]")
{
    GIVEN("a Thread running a loop that checks its stop_token")
    {
        std::atomic<bool> started{false};
        std::atomic<bool> stopped{false};

        portal::Thread thread(
                portal::ThreadProperties{.name = "stoppable-thread"},
                [&](const std::stop_token& st)
                {
                    started.store(true, std::memory_order_relaxed);
                    while (!st.stop_requested())
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                    stopped.store(true, std::memory_order_relaxed);
                }
        );

        while (!started.load(std::memory_order_relaxed))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        WHEN("request_stop is called")
        {
            const bool accepted = thread.request_stop();
            thread.join();

            THEN("the callable observes the stop request and exits")
            {
                REQUIRE(accepted);
                REQUIRE(stopped.load(std::memory_order_relaxed));
                REQUIRE_FALSE(thread.joinable());
            }
        }
    }
}

SCENARIO("Destroying a joinable Thread cancels and joins it", "[unit][core][concurrency][thread_base]")
{
    GIVEN("a Thread running a loop that only exits once stop is requested")
    {
        std::atomic<bool> finished{false};

        {
            portal::Thread thread(
                    portal::ThreadProperties{.name = "raii-thread"},
                    [&](const std::stop_token& st)
                    {
                        while (!st.stop_requested())
                        {
                            std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        }
                        finished.store(true, std::memory_order_relaxed);
                    }
            );

            THEN("it is joinable while in scope")
            {
                REQUIRE(thread.joinable());
            }
        }

        THEN("its destructor requested a stop and joined synchronously")
        {
            REQUIRE(finished.load(std::memory_order_relaxed));
        }
    }
}

SCENARIO(
        "Move-assigning a Thread cancels and joins the previously held thread",
        "[unit][core][concurrency][thread_base]"
)
{
    GIVEN("two independently running Threads")
    {
        std::atomic<bool> first_stopped{false};
        std::atomic<bool> second_started{false};

        portal::Thread first(
                portal::ThreadProperties{.name = "first"},
                [&](const std::stop_token& st)
                {
                    while (!st.stop_requested())
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                    first_stopped.store(true, std::memory_order_relaxed);
                }
        );

        portal::Thread second(
                portal::ThreadProperties{.name = "second"},
                [&](const std::stop_token& st)
                {
                    second_started.store(true, std::memory_order_relaxed);
                    while (!st.stop_requested())
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                }
        );

        while (!second_started.load(std::memory_order_relaxed))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        WHEN("first is move-assigned from second")
        {
            first = std::move(second);

            THEN("the original thread held by first was stopped and joined before the move")
            {
                REQUIRE(first_stopped.load(std::memory_order_relaxed));
            }

            AND_THEN("first now owns second's underlying thread")
            {
                REQUIRE(first.joinable());
                REQUIRE(first.get_name() == "second");

                first.request_stop();
                first.join();
            }
        }
    }
}

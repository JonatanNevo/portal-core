//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <catch2/catch_all.hpp>

import portal.core;
import std;

SCENARIO("SpinLock try_lock acquires an uncontended lock", "[unit][core][concurrency][spin_lock]")
{
    GIVEN("a fresh SpinLock")
    {
        portal::SpinLock lock;

        THEN("try_lock succeeds")
        {
            REQUIRE(lock.try_lock());
            lock.unlock();
        }
    }
}

SCENARIO("SpinLock try_lock fails while already held", "[unit][core][concurrency][spin_lock]")
{
    GIVEN("a SpinLock held by the current thread")
    {
        portal::SpinLock lock;
        REQUIRE(lock.try_lock());

        THEN("a second try_lock fails")
        {
            REQUIRE_FALSE(lock.try_lock());
        }

        AND_THEN("after unlock, try_lock succeeds again")
        {
            lock.unlock();
            REQUIRE(lock.try_lock());
            lock.unlock();
        }
    }
}

SCENARIO("SpinLock lock blocks until the lock is available", "[unit][core][concurrency][spin_lock]")
{
    GIVEN("a SpinLock held by another thread")
    {
        portal::SpinLock lock;
        lock.lock();

        std::atomic<bool> acquired{false};
        std::thread waiter(
                [&]
                {
                    lock.lock();
                    acquired.store(true, std::memory_order_relaxed);
                    lock.unlock();
                }
        );

        THEN("the waiting thread only acquires it after the holder unlocks")
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            REQUIRE_FALSE(acquired.load(std::memory_order_relaxed));

            lock.unlock();
            waiter.join();
            REQUIRE(acquired.load(std::memory_order_relaxed));
        }
    }
}

SCENARIO("SpinLock works with std::lock_guard", "[unit][core][concurrency][spin_lock]")
{
    GIVEN("a SpinLock used as a lock_guard mutex")
    {
        portal::SpinLock lock;

        THEN("the lock is released when the guard goes out of scope")
        {
            {
                std::lock_guard guard(lock);
                REQUIRE_FALSE(lock.try_lock());
            }
            REQUIRE(lock.try_lock());
            lock.unlock();
        }
    }
}

SCENARIO("SpinLock provides mutual exclusion under contention", "[unit][core][concurrency][spin_lock]")
{
    GIVEN("many threads incrementing a shared counter under a SpinLock")
    {
        constexpr int thread_count = 8;
        constexpr int increments_per_thread = 10'000;

        portal::SpinLock lock;
        long counter = 0;

        std::vector<std::thread> threads;
        threads.reserve(thread_count);
        for (int i = 0; i < thread_count; ++i)
        {
            threads.emplace_back(
                    [&]
                    {
                        for (int j = 0; j < increments_per_thread; ++j)
                        {
                            std::lock_guard guard(lock);
                            ++counter;
                        }
                    }
            );
        }

        for (auto& thread: threads)
        {
            thread.join();
        }

        THEN("the final count reflects every increment with no lost updates")
        {
            REQUIRE(counter == static_cast<long>(thread_count) * increments_per_thread);
        }
    }
}

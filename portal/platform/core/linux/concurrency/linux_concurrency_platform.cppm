//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#include <pthread.h>
#include "portal/core/log_macros.h"

export module portal.core.concurrency.platform:linux;

import std.compat;
import portal.core.concurrency;
import portal.core.log;

namespace portal
{
DECLARE_LOGGER("portal.concurrency.linux");
}

export namespace portal
{

class LinuxThread final : public ThreadBase
{
public:
    template<typename F, typename... Args>
    explicit LinuxThread(const ThreadProperties& spec, F&& f, Args&&... args) : ThreadBase(spec)
    {
        auto callable = make_callable(std::forward<F>(f), std::forward<Args>(args)...);

        thread = std::jthread(
                [name = spec.name,
                 affinity = spec.affinity,
                 priority = spec.priority,
                 core = spec.core,
                 callable = std::move(callable)](std::stop_token st) mutable
                {
                    set_name(name);
                    set_affinity(affinity, core);
                    set_priority(priority);

                    callable(st);
                }
        );
    }

protected:
    static void set_name(const std::string& name)
    {
        ThreadBase::set_name(name);
        const auto result = pthread_setname_np(pthread_self(), name.c_str());
        if (result != 0)
            LOGGER_ERROR("Failed to set the thread name with an error: {}", std::generic_category().message(result));
    }

    static void set_affinity(ThreadAffinity affinity, uint16_t core)
    {
        if (affinity == ThreadAffinity::Core || affinity == ThreadAffinity::CoreLean)
        {
            if (affinity == ThreadAffinity::CoreLean)
                LOGGER_WARN("Linux does not support lean affinity, using hard affinity instead");

            cpu_set_t cpu_set;
            CPU_ZERO(&cpu_set);
            CPU_SET(core, &cpu_set);
            const auto result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set);
            if (result != 0)
                LOGGER_ERROR("Failed to set thread affinity with result: {}", std::generic_category().message(result));
        }
    }

    static void set_priority(ThreadPriority priority)
    {
        int policy = -1;
        sched_param param{};

        const auto result_get = pthread_getschedparam(pthread_self(), &policy, &param);
        if (result_get != 0)
        {
            LOGGER_ERROR(
                    "Failed to get thread scheduling parameters with error: {}",
                    std::generic_category().message(result_get)
            );
            return;
        }

        const int min_priority = sched_get_priority_min(policy);
        const int max_priority = sched_get_priority_max(policy);
        const int mid_priority = param.sched_priority;

        switch (priority)
        {
            case ThreadPriority::Low:
                param.sched_priority = min_priority;
                break;
            case ThreadPriority::Default:
                param.sched_priority = mid_priority;
                break;
            case ThreadPriority::High:
                param.sched_priority = max_priority;
                break;
        }

        const auto result_set = pthread_setschedparam(pthread_self(), policy, &param);
        if (result_set != 0)
        {
            LOGGER_ERROR("Failed to set thread priority with error: {}", std::generic_category().message(result_set));
        }
    }
};
} // namespace portal

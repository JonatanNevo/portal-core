//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#include <mach/mach_error.h>
#include <mach/thread_act.h>
#include <mach/thread_policy.h>
#include <pthread.h>
#include "portal/core/log_macros.h"

export module portal.core.concurrency.platform:macos;

import std.compat;
import portal.core.concurrency;
import portal.core.log;

namespace portal
{
DECLARE_LOGGER("portal.concurrency.macos")
}

export namespace portal
{

class MacOSThread final : public ThreadBase
{
public:
    template<typename F, typename... Args>
    explicit MacOSThread(const ThreadSpecification& spec, F&& f, Args&&... args) : ThreadBase(spec)
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
        const auto result = pthread_setname_np(name.c_str());
        if (result != 0)
            LOGGER_ERROR("Failed to set the thread name with an error: {}", std::generic_category().message(result));
    }

    static void set_affinity(const ThreadAffinity affinity, const uint16_t core)
    {
        if (affinity == ThreadAffinity::Core || affinity == ThreadAffinity::CoreLean)
            LOGGER_DEBUG("MacOs does not support thread affinity, skipping...");
    }

    // TODO: use QOS_CLASS instead?
    static void set_priority(const ThreadPriority priority)
    {
        int policy;
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

        // Get the min and max priority for the current policy
        const int min_priority = sched_get_priority_min(policy);
        const int max_priority = sched_get_priority_max(policy);
        const int mid_priority = (min_priority + max_priority) / 2;

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
            LOGGER_ERROR("Failed to set thread priority with error: {}", std::generic_category().message(result_set));
    }
};
} // namespace portal

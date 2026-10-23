//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#include <processthreadsapi.h>
#include <windows.h>
#include "portal/core/log_macros.h"

export module portal.core.concurrency.platform:windows;

import std.compat;
import portal.core.concurrency;
import portal.core.log;

namespace portal
{
DECLARE_LOGGER("portal.concurrency.windows")
}

export namespace portal
{

class WindowsThread final : public ThreadBase
{
public:
    template<typename F, typename... Args>
    explicit WindowsThread(const ThreadSpecification& spec, F&& f, Args&&... args) : ThreadBase(spec)
    {
        auto wide_name = details::to_wstring(spec.name);
        auto callable = make_callable(std::forward<F>(f), std::forward<Args>(args)...);

        thread = std::jthread(
                [name = spec.name,
                 wide_name = wide_name,
                 affinity = spec.affinity,
                 priority = spec.priority,
                 core = spec.core,
                 callable = std::move(callable)](std::stop_token st) mutable
                {
                    set_name(name, wide_name);
                    set_affinity(affinity, core);
                    set_priority(priority);

                    callable(st);
                }
        );
    }

protected:
    static void set_name(const std::string& name, const std::wstring& wide_name)
    {
        ThreadBase::set_name(name);
        const auto result = SetThreadDescription(GetCurrentThread(), wide_name.c_str());
        if (FAILED(result))
            LOGGER_ERROR("Failed to set thread name with result: {}", std::system_category().message(result));
    }

    static void set_affinity(const ThreadAffinity affinity, const uint16_t core)
    {
        if (affinity == ThreadAffinity::Core)
        {
            const DWORD_PTR affinity_mask = DWORD_PTR{1} << core;
            const auto result = SetThreadAffinityMask(GetCurrentThread(), affinity_mask);
            if (result == 0)
            {
                const auto error = GetLastError();
                LOGGER_ERROR("Failed to set thread affinity with result: {}", std::system_category().message(error));
            }
        }
        if (affinity == ThreadAffinity::CoreLean)
        {
            const auto result = SetThreadIdealProcessor(GetCurrentThread(), core);
            if (result == static_cast<DWORD>(-1))
            {
                const auto error = GetLastError();
                LOGGER_ERROR("Failed to set thread affinity with result: {}", std::system_category().message(error));
            }
        }
    }

    static void set_priority(const ThreadPriority priority)
    {
        int priority_num = THREAD_PRIORITY_NORMAL;
        switch (priority)
        {
            case ThreadPriority::Low:
                priority_num = THREAD_PRIORITY_BELOW_NORMAL;

                break;
            case ThreadPriority::Default:
                priority_num = THREAD_PRIORITY_NORMAL;
                break;
            case ThreadPriority::High:
                priority_num = THREAD_PRIORITY_HIGHEST;
                break;
        }

        const auto result = SetThreadPriority(GetCurrentThread(), priority_num);
        if (!result)
        {
            const auto error = GetLastError();
            LOGGER_ERROR("Failed to set thread priority with result: {}", std::system_category().message(error));
        }
    }
};
} // namespace portal

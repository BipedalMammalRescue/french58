#pragma once

#include "EngineCore/Runtime/crash_dump.h"

namespace Engine::Core::Runtime {

using GenericTaskDelegate = CallbackResult(*)(void* state);

class ITaskScheduler
{
public:
    virtual void ScheduleTask(GenericTaskDelegate routine, void* state) = 0;
};

}
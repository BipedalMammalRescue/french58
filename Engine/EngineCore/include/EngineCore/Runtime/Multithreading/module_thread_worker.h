#pragma once

#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/module_definition.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/event_stream.h"
#include "EngineCore/Runtime/event_writer.h"
#include "EngineCore/Runtime/service_table.h"

#include <blockingconcurrentqueue.h>
#include <SDL3/SDL_thread.h>

namespace Engine::Core::Runtime::MultiThreading
{

enum class ModuleWorkType
{
    Undecided,
    ProcessInputEvents,
    ThreadShutDown
};

struct ModuleWork
{
    ModuleWorkType Type;

    union {
        struct {
            EventWriter* EventWriters;
            size_t EventWriterCount;
        } ProcessInputEvents;
    } Payload;
};

// A synchronous thread worker used to execute module-local state changes in parallel.
class ModuleThreadWorker
{
private:
    friend class ModuleThreadWorkerCluster;

    struct TinyConcurrentQueue : moodycamel::ConcurrentQueueDefaultTraits
    {
        static const size_t BLOCK_SIZE = 2;
    };

    ServiceTable* m_ServiceTable;
    void* m_ModuleState;
    const Pipeline::ModuleDefinition* m_ModuleDefinition;

    // multi-threaded work    
    moodycamel::BlockingConcurrentQueue<ModuleWork, TinyConcurrentQueue> m_WorkQueue = moodycamel::BlockingConcurrentQueue<ModuleWork, TinyConcurrentQueue>(1);
    moodycamel::BlockingConcurrentQueue<CallbackResult, TinyConcurrentQueue> m_ResultQueue = moodycamel::BlockingConcurrentQueue<CallbackResult, TinyConcurrentQueue>(1);
    SDL_Thread* m_Thread;
    CallbackResult m_LastWorkResult;

    Logging::Logger m_Logger;
    
    static int ThreadRoutine(void* state);

public:
    ~ModuleThreadWorker();

    CallbackResult Start(ServiceTable* services, void* moduleState, const Pipeline::ModuleDefinition* moduleDef);
    CallbackResult Join();

    // schedule work should always be called with a finish work
    CallbackResult ScheduleWork(const ModuleWork& work);

    // schedule work should always be called with a finish work
    CallbackResult ScheduleWork(const ModuleWork&& work);

    // waits for the thread to finish the previous posted work
    void FinishWork();

    inline const CallbackResult* GetWorkResult() const
    {
        return &m_LastWorkResult;
    }
};

}
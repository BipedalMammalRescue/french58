#pragma once

#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/engine_callback.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/module_manager.h"
#include "EngineCore/Runtime/task_scheduler.h"
#include "SDL3/SDL_thread.h"
#include "blockingconcurrentqueue.h"
#include "lightweightsemaphore.h"

namespace Engine::Core::Runtime {
    
class EventWriter;

enum class TaskType 
{
    ShutDown,
    ProcessInputEvents,
    GenericTask
};

struct Task 
{
    TaskType Type;

    union {
        struct {
            InstancedEventCallback Routine;
            EventWriter* EventWriters; 
            size_t EventWriterCount;
        } ProcessInputEventsTask;

        struct {
            GenericTaskDelegate Routine;
            void* State;
        } GenericTask;
    } Payload;
};

struct TaskResult 
{
    CallbackResult Result;
    size_t AdditionalTasks;
};

class TaskManager
{
private:
    std::vector<SDL_Thread*> m_WorkerThreads;

    moodycamel::LightweightSemaphore m_TerminationSignal;
    moodycamel::BlockingConcurrentQueue<Task> m_TaskQueue;
    moodycamel::BlockingConcurrentQueue<TaskResult> m_ResultQueue;

    ServiceTable* m_ServiceTable;
    Logging::Logger m_Logger;

    static int ThreadRoutine(void* state);

public:
    TaskManager(ServiceTable* services, Logging::LoggerService* loggerService, size_t workerCount);
    ~TaskManager();

    inline void ScheduleWork(const Task& task)
    {
        m_TaskQueue.enqueue(task);
    }
    
    inline void ScheduleWork(const Task&& task)
    {
        m_TaskQueue.enqueue(task);
    }

    inline TaskResult WaitOne()
    {
        TaskResult result;
        m_ResultQueue.wait_dequeue(result);
        return result;
    }
};

}
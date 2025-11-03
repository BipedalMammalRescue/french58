#include "EngineCore/Runtime/task_manager.h"
#include "EngineCore/Pipeline/engine_callback.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/event_writer.h"
#include "EngineCore/Runtime/service_table.h"
#include "SDL3/SDL_thread.h"

using namespace Engine::Core::Runtime;

static const char* LogChannels[] = { "TaskManager" };

TaskManager::TaskManager(Engine::Core::Runtime::ServiceTable* services, size_t workerCount)
    : m_ServiceTable(services), m_Logger(services->LoggerService->CreateLogger(LogChannels, 1))
{
    m_WorkerThreads.reserve(workerCount);
    for (size_t i = 0; i < workerCount; i++)
    {
        SDL_Thread* newThread = SDL_CreateThread(ThreadRoutine, "Worker Thread", this);
        m_WorkerThreads.push_back(newThread);
    }
}

TaskManager::~TaskManager()
{
    // shut down all threads 
    for (size_t i = 0; i < m_WorkerThreads.size(); i++)
    {
        m_TaskQueue.enqueue({ TaskType::ShutDown });
    }

    // setting a termiantion signal to skip any existing tasks
    m_TerminationSignal.signal(m_WorkerThreads.size());

    for (SDL_Thread* thread : m_WorkerThreads)
    {
        SDL_WaitThread(thread, nullptr);
    }

    m_Logger.Information("Worker threads exited.");
}

static CallbackResult ProcessInputEvent(const ServiceTable* services, void* moduleState, CallbackResult (*callback)(const ServiceTable* services, void* moduleState, EventStream eventStreams), EventWriter* eventWriters, size_t eventWriterCount)
{
    for (size_t i = 0; i < eventWriterCount; i++)
    {
        CallbackResult result = callback(services, moduleState, eventWriters[i].OpenReadStream());
        if (result.has_value())
            return result;
    }

    return CallbackSuccess();
}

int TaskManager::ThreadRoutine(void* state)
{
    auto taskManager = (TaskManager*)state;

    static const char* ThreadLogChannels[] = { "TaskWorker" };
    Logging::Logger logger = taskManager->m_ServiceTable->LoggerService->CreateLogger(ThreadLogChannels, 1);

    logger.Information("Task worker initiated.");

    bool keepRunning = true;
    while (keepRunning)
    {
        if (taskManager->m_TerminationSignal.tryWait())
            break;

        Task task;
        taskManager->m_TaskQueue.wait_dequeue(task);

        switch (task.Type)
        {
        case TaskType::ShutDown:
            keepRunning = false;
            break;
        case TaskType::ProcessInputEvents:
            {
                CallbackResult result = ProcessInputEvent(
                    taskManager->m_ServiceTable, 
                    task.Payload.ProcessInputEventsTask.Routine.InstanceState, 
                    task.Payload.ProcessInputEventsTask.Routine.Callback,
                    task.Payload.ProcessInputEventsTask.EventWriters,
                    task.Payload.ProcessInputEventsTask.EventWriterCount
                );
                taskManager->m_ResultQueue.enqueue({ result, 0 });
            }
            break;
        }
    }

    logger.Information("Task worker teminated.");
    return 0;
}
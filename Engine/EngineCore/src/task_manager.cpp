#include "EngineCore/Runtime/task_manager.h"
#include "EngineCore/Logging/logger_service.h"
#include "EngineCore/Pipeline/engine_callback.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/event_writer.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Runtime/task_scheduler.h"
#include "SDL3/SDL_thread.h"

using namespace Engine::Core::Runtime;

static const char* LogChannels[] = { "TaskManager" };

TaskManager::TaskManager(Engine::Core::Runtime::ServiceTable* services, Logging::LoggerService* loggerService, size_t workerCount)
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

static CallbackResult ProcessInputEvent(const ServiceTable* services, ITaskScheduler* scheduler, void* moduleState, Engine::Core::Pipeline::EventCallbackDelegate callback, EventWriter* eventWriters, size_t eventWriterCount)
{
    for (size_t i = 0; i < eventWriterCount; i++)
    {
        CallbackResult result = callback(services, scheduler, moduleState, eventWriters[i].OpenReadStream());
        if (result.has_value())
            return result;
    }

    return CallbackSuccess();
}

class TaskLocalScheduler : public ITaskScheduler
{
private:
    Engine::Core::Logging::Logger* m_Logger;
    TaskManager* m_TaskManager;
    size_t m_ScheduleCount;

public:
    TaskLocalScheduler(Engine::Core::Logging::Logger* logger, TaskManager* taskManager) 
        : m_Logger(logger), m_TaskManager(taskManager), m_ScheduleCount(0) 
    {
    }

    void ScheduleTask(GenericTaskDelegate routine, void *state) override 
    {
        Task task;
        task.Type = Engine::Core::Runtime::TaskType::GenericTask;
        task.Payload.GenericTask = { routine, state };
        m_TaskManager->ScheduleWork(task);
        m_ScheduleCount++;
    }

    size_t GetSchedulCount() const
    {
        return m_ScheduleCount;
    }
};

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
                TaskLocalScheduler localScheduler(&logger, taskManager);

                CallbackResult result = ProcessInputEvent(
                    taskManager->m_ServiceTable,
                    &localScheduler,
                    task.Payload.ProcessInputEventsTask.Routine.InstanceState, 
                    task.Payload.ProcessInputEventsTask.Routine.Callback,
                    task.Payload.ProcessInputEventsTask.EventWriters,
                    task.Payload.ProcessInputEventsTask.EventWriterCount
                );

                taskManager->m_ResultQueue.enqueue({ result, localScheduler.GetSchedulCount() });
            }
            break;
        case TaskType::GenericTask:
            {
                CallbackResult result = task.Payload.GenericTask.Routine(task.Payload.GenericTask.State);
                taskManager->m_ResultQueue.enqueue({ result, 0 });
            }
            break;
        }
    }

    logger.Information("Task worker teminated.");
    return 0;
}
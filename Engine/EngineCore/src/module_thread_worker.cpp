#include "EngineCore/Runtime/Multithreading/module_thread_worker.h"
#include "EngineCore/Pipeline/module_definition.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/service_table.h"
#include "blockingconcurrentqueue.h"

#include <SDL3/SDL_atomic.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_thread.h>
#include <string>

using namespace Engine::Core::Runtime;

static CallbackResult ProcessInputEvent(const ServiceTable* services, void* moduleState, const Engine::Core::Pipeline::ModuleDefinition* moduleDefintion, EventWriter* eventWriters, size_t eventWriterCount)
{
    for (size_t i = 0; i < moduleDefintion->EventCallbackCount; i++)
    {
        CallbackResult result = moduleDefintion->EventCallbacks[i].Callback(services, moduleState, eventWriters[i].OpenReadStream());
        if (result.has_value())
            return result;
    }

    return CallbackSuccess();
}

int MultiThreading::ModuleThreadWorker::ThreadRoutine(void* state)
{
    auto owner = (MultiThreading::ModuleThreadWorker*)state;

    owner->m_Logger.Information("Module thread worker starts (id = {moduleId}).", { owner->m_ModuleDefinition->Name });

    bool continueRunning = true;
    
    while (continueRunning)
    {
        ModuleWork work = {};
        CallbackResult result = CallbackSuccess();

        owner->m_WorkQueue.wait_dequeue(work);

        switch (work.Type)
        {
        case ModuleWorkType::Undecided:
            owner->m_Logger.Error("Unexpected work type in module thread worker (id = {module}): Undecided.", { owner->m_ModuleDefinition->Name });
            break;
        case ModuleWorkType::ProcessInputEvents:
            {
                result = ProcessInputEvent(
                    owner->m_ServiceTable, 
                    owner->m_ModuleState, 
                    owner->m_ModuleDefinition, 
                    work.Payload.ProcessInputEvents.EventWriters,
                    work.Payload.ProcessInputEvents.EventWriterCount
                );
            }
            break;
        case ModuleWorkType::ThreadShutDown:
            continueRunning = false;
            break;
        }

        owner->m_ResultQueue.enqueue(result);
    }

    return 0;
}

CallbackResult MultiThreading::ModuleThreadWorker::Start(ServiceTable* services, void* moduleState, const Pipeline::ModuleDefinition* moduleDef)
{
    m_ServiceTable = services;
    m_ModuleState = moduleState;
    m_ModuleDefinition = moduleDef;

    static const char* LogChannels[] = { "ModuleThreadWorker" };
    m_Logger = services->LoggerService->CreateLogger(LogChannels, 1);

    m_Thread = SDL_CreateThread(&MultiThreading::ModuleThreadWorker::ThreadRoutine, "Module Worker Thread", this);
    if (m_Thread == nullptr)
        return Crash(__FILE__, __LINE__, std::string("Error starting module thread worker: SDL returns null; details: ") + SDL_GetError());

    return CallbackSuccess();
}

CallbackResult MultiThreading::ModuleThreadWorker::Join()
{
    if (m_Thread == nullptr)
        return Crash(__FILE__, __LINE__, "Error joining module thread worker: thread already terminated.");

    ScheduleWork({ ModuleWorkType::ThreadShutDown });
    FinishWork();

    int status = 0;
    SDL_WaitThread(m_Thread, &status);
    m_Thread = nullptr;

    if (status != 0)
        return Crash(__FILE__, __LINE__, std::string("Error reported from joined module thread worker: status = ") + std::to_string(status));

    return CallbackSuccess();
}

CallbackResult MultiThreading::ModuleThreadWorker::ScheduleWork(const ModuleWork& work)
{
    // set the work data
    m_WorkQueue.enqueue(work);
    return CallbackSuccess();
}

CallbackResult MultiThreading::ModuleThreadWorker::ScheduleWork(const ModuleWork&& work)
{
    // set the work data
    m_WorkQueue.enqueue(work);
    return CallbackSuccess();
}

void MultiThreading::ModuleThreadWorker::FinishWork()
{
    CallbackResult result;
    m_ResultQueue.wait_dequeue(result);
    m_LastWorkResult = result;
}

MultiThreading::ModuleThreadWorker::~ModuleThreadWorker()
{
    if (m_Thread != nullptr)
    {
        Join();
    }
}
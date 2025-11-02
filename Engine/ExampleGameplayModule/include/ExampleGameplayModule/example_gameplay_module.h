#pragma once

#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/module_definition.h"
#include "EngineCore/Runtime/event_manager.h"

namespace Engine::Extension::ExampleGameplayModule
{

Core::Pipeline::ModuleDefinition GetDefinition();

struct YellEvent
{
    const char* Content;
};

struct ModuleState 
{
    std::vector<Core::Runtime::AnnotatedEvent<YellEvent>> YellEvents;
    Core::Runtime::EventOwner<YellEvent> YellOwner;
    Core::Logging::Logger Logger;
};

}
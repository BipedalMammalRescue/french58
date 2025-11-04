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
    Core::Runtime::EventOwner<YellEvent> YellOwner;
    std::vector<int> SpinningEntities;
    Core::Logging::Logger Logger;
};

}
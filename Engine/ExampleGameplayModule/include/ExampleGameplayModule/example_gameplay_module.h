#pragma once

#include "EngineCore/Logging/logger_service.h"
#include "EngineCore/Pipeline/module_definition.h"
#include "EngineCore/Runtime/event_manager.h"
#include "ExampleGameplayModule/auto_rotate_marker.h"

namespace Engine::Extension::ExampleGameplayModule
{

Core::Pipeline::ModuleDefinition GetDefinition();

struct YellEvent
{
    const char* Content;
};

class ModuleState 
{
public:
    Core::Runtime::EventOwner<YellEvent> YellOwner;
    std::vector<RotationMarker> SpinningEntities;
    Core::Logging::Logger Logger;

    ModuleState(Core::Logging::LoggerService* loggerService) : Logger(loggerService->CreateLogger("ExampleGameplayModule")) {}
};

}
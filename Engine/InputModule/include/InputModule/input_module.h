#pragma once

#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/service_table.h"
#include "InputModule/Assets/input_action.h"
#include "SDL3/SDL_keycode.h"

#include <EngineCore/Pipeline/module_definition.h>
#include <unordered_map>
#include <vector>

namespace Engine::Extension::InputModule {

Engine::Core::Pipeline::ModuleDefinition GetModuleDefinition();

class InputModuleState
{
public:
    Core::Logging::Logger Logger;

    std::unordered_map<SDL_Keycode, Assets::DiscreteInputAction> DiscreteKeyboardTriggerTable;
    std::unordered_map<SDL_Keycode, Assets::EmissionInputAction> EmissionKeyboardTriggerTable;

    // output section
    std::vector<Core::Pipeline::HashId> DiscreteActivations;
    std::vector<Core::Pipeline::HashId> DiscreteDeactivations;
    std::unordered_map<Core::Pipeline::HashId, float> Emissions;

    InputModuleState(Core::Runtime::ServiceTable* services);
};

}
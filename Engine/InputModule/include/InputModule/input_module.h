#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include "InputModule/Assets/input_action.h"
#include "SDL3/SDL_keycode.h"

#include <EngineCore/Pipeline/module_definition.h>
#include <unordered_map>
#include <vector>

namespace Engine::Extension::InputModule {

Engine::Core::Pipeline::ModuleDefinition GetModuleDefinition();

struct InputModuleState
{
    std::unordered_map<SDL_Keycode, Assets::DiscreteInputAction> DiscreteKeyboardTriggerTable;
    std::unordered_map<SDL_Keycode, Assets::EmissionInputAction> EmissionKeyboardTriggerTable;

    // output section
    std::vector<Core::Pipeline::HashId> DiscreteActivations;
    std::vector<Core::Pipeline::HashId> DiscreteDeactivations;
    std::unordered_map<Core::Pipeline::HashId, float> Emissions;
};

}
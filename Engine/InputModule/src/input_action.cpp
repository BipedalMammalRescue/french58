#include "InputModule/Assets/input_action.h"
#include "EngineCore/Pipeline/asset_enumerable.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "InputModule/input_action_type.h"
#include "InputModule/input_component_type.h"
#include "InputModule/input_module.h"

using namespace Engine;
using namespace Engine::Extension::InputModule::Assets;

Core::Runtime::CallbackResult Engine::Extension::InputModule::Assets::LoadInputAction(
    Core::Pipeline::IAssetEnumerator *inputStreams, 
    Core::Runtime::ServiceTable *services, void *moduleState)
{
    InputModuleState* state = static_cast<InputModuleState*>(moduleState);

    // load the input actions
    while (inputStreams->MoveNext())
    {
        // skip the input action name for now
        unsigned int nameLen = 0;
        inputStreams->GetCurrent().Storage->read((char*)&nameLen, sizeof(int));
        inputStreams->GetCurrent().Storage->seekg(nameLen, std::ios::cur);

        InputActionType actionType;
        inputStreams->GetCurrent().Storage->read((char*)&actionType, sizeof(InputActionType));

        switch (actionType)
        {
        case InputActionType::Discrete:
            {
                DiscreteInputAction action { inputStreams->GetCurrent().ID };
                inputStreams->GetCurrent().Storage->read((char*)&action.Trigger.Type, sizeof(action.Trigger.Type));
                inputStreams->GetCurrent().Storage->read((char*)&action.Trigger.Identifier, sizeof(action.Trigger.Identifier));

                switch (action.Trigger.Type)
                {
                case InputComponentType::Keyboard:
                    state->DiscreteKeyboardTriggerTable[action.Trigger.Identifier] = action;
                    break;
                }
            }
            break;
        case InputActionType::Emission:
            {
                EmissionInputAction action { inputStreams->GetCurrent().ID };
                inputStreams->GetCurrent().Storage->read((char*)&action.Trigger.Type, sizeof(action.Trigger.Type));
                inputStreams->GetCurrent().Storage->read((char*)&action.Trigger.Identifier, sizeof(action.Trigger.Identifier));

                state->Emissions[inputStreams->GetCurrent().ID] = 0;

                switch (action.Trigger.Type)
                {
                case InputComponentType::Keyboard:
                    state->EmissionKeyboardTriggerTable[action.Trigger.Identifier] = action;
                    break;
                }
            }
            break;
        }
    }

    return Core::Runtime::CallbackSuccess();
}

Core::Runtime::CallbackResult Extension::InputModule::Assets::UnloadInputAction(
    Core::Pipeline::HashId *ids, 
    size_t count, 
    Core::Runtime::ServiceTable *services, void *moduleState)
{
    return Core::Runtime::CallbackSuccess();
}
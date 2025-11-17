#include "InputModule/Assets/input_action.h"
#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineUtils/Memory/memstream_lite.h"
#include "InputModule/input_action_type.h"
#include "InputModule/input_component_type.h"
#include "InputModule/input_module.h"

using namespace Engine;
using namespace Engine::Extension::InputModule::Assets;

Core::Runtime::CallbackResult Engine::Extension::InputModule::Assets::ContextualizeInputAction(
    Core::Runtime::ServiceTable *services, void *moduleState, 
    Core::AssetManagement::AssetLoadingContext* outContext, size_t contextCount)
{
    // load input actions to the transient buffer
    for (size_t i = 0; i < contextCount; i++)
    {
        Core::AssetManagement::AssetLoadingContext& currentContext = outContext[i];
        currentContext.Buffer.Type = Core::AssetManagement::LoadBufferType::TransientBuffer;
        currentContext.Buffer.Location.TransientBufferSize = currentContext.SourceSize;
    }

    return Core::Runtime::CallbackSuccess();
}

Core::Runtime::CallbackResult Engine::Extension::InputModule::Assets::IndexInputAction(Core::Runtime::ServiceTable *services, void *moduleState, Core::AssetManagement::AssetLoadingContext* inContext)
{
    auto state = static_cast<Engine::Extension::InputModule::InputModuleState*>(moduleState);

    void* buffer = services->TransientAllocator->GetBuffer(inContext->Buffer.Location.TransientBufferId);
    if (buffer == nullptr)
    {
        state->Logger.Error("Failed to index input action {}, transient buffer is invalid.", inContext->AssetId);
        return Core::Runtime::CallbackSuccess();
    }

    Utils::Memory::MemStreamLite stream { buffer, 0 };

    // skip the input action name for now
    unsigned int nameLen = stream.Read<int>();
    stream.Cursor += nameLen;

    InputActionType actionType = stream.Read<InputActionType>();

    switch (actionType)
    {
    case InputActionType::Discrete:
        {
            DiscreteInputAction action { 
                inContext->AssetId,
                {
                    stream.Read<InputComponentType>(),
                    stream.Read<unsigned int>()
                }
            };

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
            EmissionInputAction action {
                inContext->AssetId,
                {
                    stream.Read<InputComponentType>(),
                    stream.Read<unsigned int>()
                }
            };

            state->Emissions[action.Id] = 0;

            switch (action.Trigger.Type)
            {
            case InputComponentType::Keyboard:
                state->EmissionKeyboardTriggerTable[action.Trigger.Identifier] = action;
                break;
            }
        }
        break;
    }

    return Core::Runtime::CallbackSuccess();
}
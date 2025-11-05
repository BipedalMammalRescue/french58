#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include "InputModule/input_component_type.h"

#include "EngineCore/Pipeline/fwd.h"
#include "EngineCore/Runtime/fwd.h"
#include "EngineCore/Runtime/crash_dump.h"

namespace Engine::Extension::InputModule::Assets {

Core::Runtime::CallbackResult LoadInputAction(Core::Pipeline::IAssetEnumerator *inputStreams, Core::Runtime::ServiceTable *services, void *moduleState);
Core::Runtime::CallbackResult UnloadInputAction(Core::Pipeline::HashId *ids, size_t count, Core::Runtime::ServiceTable *services, void *moduleState);

// input component points to a piece of information that input module can query; it doesn't carry the schema or interpretation
struct InputComponent
{
    Engine::Extension::InputModule::InputComponentType Type;
    unsigned int Identifier;
};

// discrete input actions interprets any input components as digital switches
struct DiscreteInputAction
{
    Core::Pipeline::HashId Id;
    InputComponent Trigger;
    bool CachedState;
};

// emission input action interprets any input components as a range from 0.0 to 1.0
struct EmissionInputAction
{
    Core::Pipeline::HashId Id;
    InputComponent Trigger;
};

}
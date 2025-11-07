#pragma once

#include "EngineCore/Scripting/param_enumerator.h"
#include "EngineCore/Scripting/return_writer.h"
#include "EngineCore/Scripting/script_object.h"

namespace Engine::Core::Runtime {

struct ServiceTable;

}

namespace Engine::Core::Scripting {

enum class ScriptCallableResult : unsigned char
{
    Success,
    ParamMismatch,
    OtherException
};

struct ScriptCallable
{
    const char* Name;

    const ScriptObject* ParamType;
    const ScriptObject* ReturnType;

    ScriptCallableResult (*Delegate)(const Runtime::ServiceTable* services, const void* moduleState, Engine::Core::Scripting::IParamReader* param, IReturnWriter* output);
};

}
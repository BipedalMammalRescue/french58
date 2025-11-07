#pragma once

#include "EngineCore/Scripting/macros.h"
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

    const ScriptObject** ParamTypes;
    size_t ParamCount;

    const ScriptObject* ReturnType;

    ScriptCallableResult (*Delegate)(const Runtime::ServiceTable* services, const void* moduleState, IParamEnumerator* params, IReturnWriter* output);
};

template <typename TRet, typename TP1>
inline ScriptCallable GetDemoScriptCallable(const char* name, TRet(*delegate)(TP1))
{
    const ScriptObject* paramTypes[] = {
        GetScriptObjectForType<TP1>()
    };

    ScriptCallable callable = {
        name,
        paramTypes,
        1,
        GetScriptObjectForType<TRet>(),
        
    };

    return callable;
}

}
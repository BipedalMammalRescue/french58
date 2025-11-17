#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Scripting/api_query.h"
#include "EngineCore/Scripting/api_event.h"
#include <md5.h>

namespace Engine::Extension::LuaScriptingModule {

class InstancedScriptParamId
{
public:
    Core::Pipeline::HashId Name;
    int ComponentId;

    inline bool operator==(const InstancedScriptParamId& other) const 
    {
        return Name == other.Name && ComponentId == other.ComponentId;
    }
};

struct InstancedScriptNode
{
    int Component;
    int Entity;
    int ScriptIndex;
};

struct InstancedApiQuery
{
    const void* ModuleState;
    const Core::Scripting::ApiQueryBase* Api;
};

struct InstancedApiEvent
{
    const void* ModuleState;
    const Core::Scripting::ApiEventBase* Api;
};

}

template <>
struct std::hash<Engine::Extension::LuaScriptingModule::InstancedScriptParamId>
{
    std::size_t operator()(const Engine::Extension::LuaScriptingModule::InstancedScriptParamId& k) const
    {
        return k.Name.LowQuad() + k.ComponentId;
    }
};
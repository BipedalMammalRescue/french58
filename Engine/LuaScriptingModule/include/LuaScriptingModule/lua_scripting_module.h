#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/module_definition.h"
#include "LuaScriptingModule/lua_executor.h"
#include <unordered_map>
#include "LuaScriptingModule/state_data.h"

namespace Engine::Extension::LuaScriptingModule {

Engine::Core::Pipeline::ModuleDefinition GetModuleDefinition();

class LuaScriptingModuleState 
{
private:
    std::unordered_map<Core::Pipeline::HashId, InstancedScript> LoadedScripts;
    LuaExecutor m_Executor;

public:
    LuaScriptingModuleState(const Engine::Core::Runtime::ServiceTable* services) : m_Executor(services) {}

    inline std::unordered_map<Core::Pipeline::HashId, InstancedScript>& GetLoadedScripts() 
    {
        return LoadedScripts;
    }

    inline const std::unordered_map<Core::Pipeline::HashId, InstancedScript>& GetLoadedScripts() const 
    {
        return LoadedScripts;
    }

    inline LuaExecutor* GetExecutor() 
    {
        return &m_Executor;
    }
};

}
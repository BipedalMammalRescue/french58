#pragma once

#include "EngineCore/Logging/logger.h"
#include "EngineCore/Logging/logger_service.h"
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
    Core::Logging::Logger m_Logger;
    std::unordered_map<Core::Pipeline::HashId, int> m_LoadedScripts;
    std::vector<InstancedScriptNode> m_ScriptNodes;

    LuaExecutor m_Executor;
    int m_ScriptCounter;

public:
    LuaScriptingModuleState(const Engine::Core::Runtime::ServiceTable* services) : 
        m_Logger(services->LoggerService->CreateLogger("LuaScriptingModule")), 
        m_Executor(services), 
        m_ScriptCounter(0)
    {
        m_Executor.Initialize();
    }

    inline std::unordered_map<Core::Pipeline::HashId, int>& GetLoadedScripts() 
    {
        return m_LoadedScripts;
    }

    inline const std::unordered_map<Core::Pipeline::HashId, int>& GetLoadedScripts() const 
    {
        return m_LoadedScripts;
    }

    inline std::vector<InstancedScriptNode>& GetNodes()
    {
        return m_ScriptNodes;
    }

    inline const std::vector<InstancedScriptNode>& GetNodes() const
    {
        return m_ScriptNodes;
    }

    inline LuaExecutor* GetExecutor() 
    {
        return &m_Executor;
    }

    inline int IncrementScriptCounter()
    {
        m_ScriptCounter ++;
        return m_ScriptCounter;
    }

    inline Core::Logging::Logger* GetLogger()
    {
        return &m_Logger;
    }
};

}
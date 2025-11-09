#include "LuaScriptingModule/lua_scripting_module.h"
#include "EngineCore/Pipeline/asset_definition.h"
#include "EngineCore/Pipeline/module_definition.h"
#include "EngineCore/Runtime/event_manager.h"
#include "EngineCore/Runtime/module_manager.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Scripting/api_query.h"
#include "LuaScriptingModule/Assets/lua_script.h"
#include "LuaScriptingModule/api.h"
#include "LuaScriptingModule/lua_executor.h"

using namespace Engine::Extension::LuaScriptingModule;

#define MODULE_NAME HASH_NAME("LuaScriptingModule")

void ScriptNodeEventSystem(const Engine::Core::Runtime::ServiceTable* services, void* localState, Engine::Core::Runtime::EventWriter* writer)
{
    auto moduleState = static_cast<const LuaScriptingModuleState*>(services->ModuleManager->FindModule(MODULE_NAME.Hash));
    auto executor = static_cast<LuaExecutor*>(localState);

    for (const auto& scriptGroup : moduleState->GetLoadedScripts())
    {
        executor->LoadScript(&scriptGroup.second.Code);

        for (const auto& node : scriptGroup.second.Nodes)
        {
            executor->ExecuteNode(node, writer);
        }
    }
}

void* InitializeModule(Engine::Core::Runtime::ServiceTable* services)
{
    auto newState = new LuaScriptingModuleState(services);
    
    Engine::Core::Runtime::EventSystemInstance newSystem {
        ScriptNodeEventSystem,
        "ScriptNodeExecutor",
        newState->GetExecutor()
    };

    services->EventManager->RegisterEventSystem(&newSystem, 1);

    return newState;
}

void DisposeModule(Engine::Core::Runtime::ServiceTable* services, void* moduleState)
{
    delete static_cast<LuaScriptingModuleState*>(moduleState);
}

Engine::Core::Pipeline::ModuleDefinition Engine::Extension::LuaScriptingModule::GetModuleDefinition()
{
    static const Core::Scripting::ApiQueryBase* apis[] = {
        TestApi::GetQuery()
    };

    static const Core::Pipeline::AssetDefinition assets[] = {
        {
            HASH_NAME("ScriptNode"),
            Assets::LoadLuaScript,
            Assets::UnloadLuaScript,
        }
    };

    return Core::Pipeline::ModuleDefinition 
    {
        MODULE_NAME,
        InitializeModule,
        DisposeModule,
        assets,
        sizeof(assets) / sizeof(Core::Pipeline::AssetDefinition),
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        apis,
        sizeof(apis) / sizeof(void*)
    };
}
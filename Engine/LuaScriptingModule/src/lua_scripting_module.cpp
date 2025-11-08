#include "LuaScriptingModule/lua_scripting_module.h"
#include "EngineCore/Pipeline/module_definition.h"
#include "EngineCore/Runtime/service_table.h"

using namespace Engine::Extension::LuaScriptingModule;

void* InitializeModule(Engine::Core::Runtime::ServiceTable* services)
{
    return nullptr;
}

void DisposeModule(Engine::Core::Runtime::ServiceTable* services, void* moduleState)
{

}

Engine::Core::Pipeline::ModuleDefinition Engine::Extension::LuaScriptingModule::GetModuleDefinition()
{
    return Core::Pipeline::ModuleDefinition 
    {
        HASH_NAME("LuaScriptingModule"),
        InitializeModule,
        DisposeModule,
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0
    };
}
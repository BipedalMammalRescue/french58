#include "LuaScriptingModule/lua_api_builder.h"
#include "EngineCore/Pipeline/module_assembly.h"
#include "EngineCore/Pipeline/module_definition.h"
#include "lua.h"

using namespace Engine::Extension::LuaScriptingModule;

LuaApiBuilder::LuaApiBuilder(lua_State* luaState)
{
    Core::Pipeline::ModuleAssembly modules = Core::Pipeline::ListModules();

    lua_createtable(luaState, 0, modules.ModuleCount);

    for (size_t i = 0; i < modules.ModuleCount; i++)
    {
        const Core::Pipeline::ModuleDefinition* module = modules.Modules + i;

        if (module->ApiCallableCount <= 0)
            continue;

        lua_pushstring(luaState, module->Name.DisplayName);
        lua_createtable(luaState, 0, module->ApiCallableCount);

        for (size_t j = 0; j < module->ApiCallableCount; j++)
        {
            lua_pushstring(luaState, module->ApiCallables[j]->Name);
            lua_pushinteger(luaState, m_Callables.size());
            lua_settable(luaState, -3);

            m_Callables.push_back(module->ApiCallables[j]);
        }

        lua_settable(luaState, -3);
    }

    lua_setglobal(luaState, "SE_ENGINE_API_TABLE");
}
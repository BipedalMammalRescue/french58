#include "EngineCore/Scripting/script_callable.h"
#include "LuaScriptingModule/lua_api_builder.h"
#include "LuaScriptingModule/lua_param_enumerator.h"
#include "LuaScriptingModule/lua_return_writer.h"

#include <cassert>
#include <lauxlib.h>
#include <lualib.h>
#include <lua.h>
#include <iostream>

using namespace Engine::Extension::LuaScriptingModule;

static int LuaApiMultiplexer(lua_State* luaState)
{
    // get the engine state
    lua_getglobal(luaState, "SE_ENGINE_STATE");
    LuaApiBuilder* state = static_cast<LuaApiBuilder*>(lua_touserdata(luaState, -1));
    
    // get the api index
    lua_getglobal(luaState, "SE_API_IDENTIFIER");
    int apiIndex = static_cast<int>(lua_tointeger(luaState, -1));

    // return nil if the api isn't found
    if (apiIndex < 0 || apiIndex >= state->GetCallable().size())
        return 0;

    // find the api
    const Engine::Core::Scripting::ScriptCallable* callable = state->GetCallable()[apiIndex];
    LuaParamReader params(callable, luaState, 2);
    LuaReturnWriter writer(callable, luaState);

    // call it
    callable->Delegate(nullptr, nullptr, &params, &writer);
    return writer.GetWriteCount();
}

static const char SetupScript[] = 
"se_engine_api = {\n\
    chosen_module = nil,\n\
    chosen_api = nil,\n\
    inner = {},\n\
    inner_proxy = {\n\
        __index = function(self, key)\n\
            SE_API_IDENTIFIER = SE_ENGINE_API_TABLE[chosen_module][key]\n\
            return SE_INVOKE\n\
        end\n\
    }\n\
}\n\
\n\
se_engine_state_access_proxy = {\n\
    __index = function(self, key)\n\
        chosen_module = key\n\
        return self.inner\n\
    end\n\
}\n\
\n\
setmetatable(se_engine_api, se_engine_state_access_proxy)\n\
setmetatable(se_engine_api.inner, se_engine_api.inner_proxy)";

int main() 
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    LuaApiBuilder builder(L);

    // add the global state used to identify api
    lua_pushinteger(L, -1);
    lua_setglobal(L, "SE_API_IDENTIFIER");

    // register the binding function
    lua_pushcfunction(L, LuaApiMultiplexer);
    lua_setglobal(L, "SE_INVOKE");
    
    // set the global state
    lua_pushlightuserdata(L, &builder);
    lua_setglobal(L, "SE_ENGINE_STATE");

    // run the setup script
    if (luaL_dostring(L, SetupScript) != LUA_OK)
    {
        std::cout << "Can't execute setup script!" << std::endl;
        return 1;
    }

    // execute script
    if (luaL_dofile(L, "test.lua") != LUA_OK) 
    {
        std::cout << "[C] Error reading script\n";
        luaL_error(L, "Error: %s\n", lua_tostring(L, -1));
    }

    lua_close(L);
}
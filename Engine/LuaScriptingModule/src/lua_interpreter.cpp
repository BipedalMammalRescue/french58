#include "EngineCore/Scripting/script_callable.h"
#include "EngineCore/Scripting/script_object.h"
#include "LuaScriptingModule/lua_data_accesses.h"
#include "EngineCore/Scripting/macros.h"
#include "EngineCore/Scripting/script_property.h"
#include "LuaScriptingModule/lua_param_enumerator.h"
#include "LuaScriptingModule/lua_return_writer.h"

#include <cassert>
#include <lauxlib.h>
#include <lualib.h>
#include <lua.h>
#include <iostream>
#include <ostream>

using namespace Engine::Extension::LuaScriptingModule;

struct ScriptEngineState
{
    std::vector<const Engine::Core::Scripting::ScriptCallable*> Callables;
};

static int LuaApiMultiplexer(lua_State* luaState)
{
    // get the engine state
    lua_getglobal(luaState, "SE_ENGINE_STATE");
    ScriptEngineState* state = static_cast<ScriptEngineState*>(lua_touserdata(luaState, -1));
    
    // get the api index
    lua_getglobal(luaState, "SE_API_IDENTIFIER");
    int apiIndex = static_cast<int>(lua_tointeger(luaState, -1));

    // return nil if the api isn't found
    if (apiIndex < 0 || apiIndex >= state->Callables.size())
        return 0;

    // find the api
    const Engine::Core::Scripting::ScriptCallable* callable = state->Callables[apiIndex];
    LuaParamEnumerator params(callable, luaState, 2);
    LuaReturnWriter writer(callable, luaState);

    // call it
    callable->Delegate(nullptr, nullptr, &params, &writer);
    return writer.GetWriteCount();
}

static const char SetupScript[] = 
"se_engine_api = {}\n\
se_engine_state_access_proxy = {\n\
    __index = function(self, key)\n\
        SE_API_IDENTIFIER = SE_FUNCTION_IDENTIFIER_TABLE[key]\n\
        return SE_API_INVOKE_FUNCTION\n\
    end\n\
}\n\
setmetatable(se_engine_api, se_engine_state_access_proxy)\n";

struct TestNestedData
{
    int Sub1;
    int Sub2;
};

DECLARE_SCRIPT_OBJECT(TestNestedData)
    ADD_MEMBER(Sub1)
    ADD_MEMBER(Sub2)
END_SCRIPT_OBJECT

struct TestData
{
    int Header;
    int Header2;
    Engine::Core::Scripting::ScriptArray<int> Member;
    int Trailer;
};

DECLARE_SCRIPT_OBJECT(TestData)
    ADD_MEMBER(Header)
    ADD_MEMBER(Header2)
    ADD_ARRAY_MEMBER(Member)
    ADD_MEMBER(Trailer)
END_SCRIPT_OBJECT

DECLARE_SCRIPT_CALLABLE(TestFunction, TestData, int, data)
{
    static const int source[] = {10, 20, 30};
    TestData result = {
        100,
        200,
        {
            source,
            3
        },
        400
    };
    return result;
}
END_SCRIPT_CALLABLE

int main() 
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    // add the global state used to identify api
    lua_pushinteger(L, -1);
    lua_setglobal(L, "SE_API_IDENTIFIER");

    // register the binding function
    lua_pushcfunction(L, LuaApiMultiplexer);
    lua_setglobal(L, "SE_API_INVOKE_FUNCTION");

    ScriptEngineState state;
    state.Callables.push_back(GetTestFunctionScriptCallable());
    
    // set the global state
    lua_pushlightuserdata(L, &state);
    lua_setglobal(L, "SE_ENGINE_STATE");

    // set the api table
    lua_createtable(L, 0, 1);
    lua_pushstring(L, GetTestFunctionScriptCallable()->Name);
    lua_pushinteger(L, 0);
    lua_settable(L, -3);
    lua_setglobal(L, "SE_FUNCTION_IDENTIFIER_TABLE");

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
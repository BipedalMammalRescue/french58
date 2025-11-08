#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
#include <cstdio>
#include <lua.hpp>
#include <iostream>


static int LuaCallable(lua_State* luaState)
{
    std::cout << "[C] stack count: " << lua_gettop(luaState) << std::endl;
    return 0;
}

int main()
{
    // set up the runtime
    lua_State* luaState = luaL_newstate();
    luaL_openlibs(luaState);

    // add function
    lua_pushcfunction(luaState, LuaCallable);
    lua_setglobal(luaState, "test_func");

    // execute
    if (!luaL_dofile(luaState, "test.lua") == LUA_OK) 
    {
        std::cout << "[C] Error reading script\n";
        printf("Error: %s\n", lua_tostring(luaState, -1));
    }

    lua_close(luaState);
}
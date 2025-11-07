#include "lua_runner.h"
#include "lauxlib.h"
#include "lualib.h"

using namespace Engine::Extension::LuaScriptingModule;

LuaRunner::LuaRunner()
{
    m_LuaState = luaL_newstate();
    luaL_openlibs(m_LuaState);
}

LuaRunner::~LuaRunner() 
{
    lua_close(m_LuaState);
}
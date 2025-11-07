#pragma once

#include "lua.h"

namespace Engine::Extension::LuaScriptingModule {

class LuaRunner
{
private:
    lua_State* m_LuaState = nullptr;

public:
    LuaRunner();
    ~LuaRunner();
};

}
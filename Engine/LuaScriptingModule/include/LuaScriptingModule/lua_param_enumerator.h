#pragma once

#include "EngineCore/Scripting/param_enumerator.h"
#include "EngineCore/Scripting/script_callable.h"
#include "lua.h"

namespace Engine::Extension::LuaScriptingModule {

class LuaParamEnumerator : public Core::Scripting::IParamReader
{
private:
    const Core::Scripting::ScriptCallable* m_Callable;
    lua_State* m_LuaState;
    int m_StackOffset;

    size_t m_Cursor = ~0;

protected:
    bool GetCore(void* destination) override;

public:
    LuaParamEnumerator(const Core::Scripting::ScriptCallable* callable, lua_State* luaState, int stackOffset) : m_Callable(callable), m_LuaState(luaState), m_StackOffset(stackOffset) {}
};

}
#pragma once

#include "EngineCore/Scripting/return_writer.h"
#include "EngineCore/Scripting/script_callable.h"
#include "lua.h"

namespace Engine::Extension::LuaScriptingModule {

class LuaReturnWriter : public Core::Scripting::IReturnWriter
{
private:
    const Core::Scripting::ScriptCallable* m_Callable;
    lua_State* m_LuaState;

    size_t m_WriteCount = 0;

protected:
    void WriteCore(const void* data, size_t size) override;

public:
    LuaReturnWriter(const Core::Scripting::ScriptCallable* callable, lua_State* state) : m_Callable(callable), m_LuaState(state) {}
    inline size_t GetWriteCount() const 
    {
        return m_WriteCount;
    }
};

}
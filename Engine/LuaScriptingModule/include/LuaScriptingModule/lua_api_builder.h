#pragma once

#include "EngineCore/Scripting/script_callable.h"
#include <lstate.h>
#include <vector>
namespace Engine::Extension::LuaScriptingModule {

class LuaApiBuilder
{
private:
    std::vector<const Core::Scripting::ScriptCallable*> m_Callables;

public:
    inline const std::vector<const Core::Scripting::ScriptCallable*>& GetCallable() const
    {
        return m_Callables;
    }

    LuaApiBuilder(lua_State* luaState);
};

}
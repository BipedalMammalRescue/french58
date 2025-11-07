#pragma once

#include "EngineCore/Scripting/macros.h"
#include "EngineCore/Scripting/script_object.h"
#include "lua.h"
#include <cstddef>

namespace Engine::Extension::LuaScriptingModule {

size_t PopScriptObject(const Engine::Core::Scripting::ScriptObject* reflection, void* data, lua_State* luaState, int index);

template <typename T>
size_t PopScriptObject(T* data, lua_State* luaState, int index)
{
    return PopScriptObject(Core::Scripting::GetScriptObjectForType<T>(), data, luaState, index);
}

size_t PushScriptObject(const Engine::Core::Scripting::ScriptObject* reflection, const void* data, lua_State* luaState);

template <typename T>
size_t PushScriptObject(T* data, lua_State* luaState)
{
    return PushScriptObject(Core::Scripting::GetScriptObjectForType<T>(), data, luaState);
}

}
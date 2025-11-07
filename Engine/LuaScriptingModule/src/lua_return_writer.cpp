#include "LuaScriptingModule/lua_return_writer.h"
#include "LuaScriptingModule/lua_data_accesses.h"

using namespace Engine::Extension::LuaScriptingModule;

void LuaReturnWriter::WriteCore(const void* data, size_t size)
{
    PushScriptObject(m_Callable->ReturnType, data, m_LuaState);
    m_WriteCount ++;
}
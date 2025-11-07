#include "LuaScriptingModule/lua_param_enumerator.h"
#include "LuaScriptingModule/lua_data_accesses.h"

using namespace Engine::Extension::LuaScriptingModule;

bool LuaParamReader::GetCore(void* destination)
{
    size_t readCount = PopScriptObject(m_Callable->ParamType, destination, m_LuaState, -1 - m_StackOffset);
    return readCount > 0;
}
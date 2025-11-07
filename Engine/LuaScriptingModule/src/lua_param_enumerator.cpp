#include "LuaScriptingModule/lua_param_enumerator.h"
#include "LuaScriptingModule/lua_data_accesses.h"

using namespace Engine::Extension::LuaScriptingModule;

bool LuaParamEnumerator::GetNextCore(void* destination)
{
    if (m_Cursor == ~0)
    {
        m_Cursor = 0;
    }
    else 
    {
        m_Cursor ++;
    }

    if (m_Cursor >= m_Callable->ParamCount)
        return false;

    int index = 0 - m_Callable->ParamCount + m_Cursor - m_StackOffset;

    // this means something went wrong in the parameters
    PopScriptObject(m_Callable->ParamTypes[m_Cursor], destination, m_LuaState, index);
    return true;
}
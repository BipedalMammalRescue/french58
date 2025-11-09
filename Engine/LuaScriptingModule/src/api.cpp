#include "LuaScriptingModule/api.h"

int Engine::Extension::LuaScriptingModule::TestApiDelegate(const Engine::Core::Runtime::ServiceTable*, const void*, const int* p1)
{
    return (*p1) << 2;
}
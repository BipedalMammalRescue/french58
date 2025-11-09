#pragma once

#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Scripting/api_declaration.h"

namespace Engine::Extension::LuaScriptingModule {

int TestApiDelegate(const Engine::Core::Runtime::ServiceTable*, const void*, const int* p1);
DECLARE_SE_API_1(TestApi, int, int, TestApiDelegate);

}
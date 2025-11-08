#include "LuaScriptingModule/lua_scripting_module.h"
#include "EngineCore/Pipeline/module_definition.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Scripting/api_declaration.h"
#include "EngineCore/Scripting/api_query.h"

using namespace Engine::Extension::LuaScriptingModule;

void* InitializeModule(Engine::Core::Runtime::ServiceTable* services)
{
    return new int();
}

void DisposeModule(Engine::Core::Runtime::ServiceTable* services, void* moduleState)
{
    delete static_cast<int*>(moduleState);
}

int TestApiDelegate(const Engine::Core::Runtime::ServiceTable*, const void*, const int* p1)
{
    return (*p1) << 2;
}
DECLARE_SE_API_1(TestApi, int, int, TestApiDelegate);

Engine::Core::Pipeline::ModuleDefinition Engine::Extension::LuaScriptingModule::GetModuleDefinition()
{
    static const Core::Scripting::ApiQueryBase* apis[] = {
        TestApi::GetQuery()
    };

    return Core::Pipeline::ModuleDefinition 
    {
        HASH_NAME("LuaScriptingModule"),
        InitializeModule,
        DisposeModule,
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        apis,
        sizeof(apis) / sizeof(void*)
    };
}
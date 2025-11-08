#pragma once

#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Scripting/api_query.h"
#include "lua.h"
#include <vector>
namespace Engine::Extension::LuaScriptingModule {



class LuaExecutor
{
private:
    struct InstancedApi
    {
        const void* ModuleState;
        const Core::Scripting::ApiQueryBase* Api;
    };

    const Engine::Core::Runtime::ServiceTable* m_Services;

    std::vector<InstancedApi> m_ApiList;
    lua_State* m_LuaState = nullptr;

    static int LuaInvoke(lua_State* luaState);

    static int L1CallMultiplexer(lua_State* luaState);

public:
    LuaExecutor(const Engine::Core::Runtime::ServiceTable* services);
    ~LuaExecutor();

    void Initialize();

    Core::Runtime::CallbackResult ExecuteFile(const char* path);
};

}
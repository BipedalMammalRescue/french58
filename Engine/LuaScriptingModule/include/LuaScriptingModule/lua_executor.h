#pragma once

#include "EngineCore/Logging/logger.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/event_writer.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Scripting/api_query.h"
#include "lua.h"
#include "LuaScriptingModule/state_data.h"
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

    Core::Logging::Logger m_Logger;

    std::vector<InstancedApi> m_ApiList;
    lua_State* m_LuaState = nullptr;

    static int LuaInvoke(lua_State* luaState);
    static int L1CallMultiplexer(lua_State* luaState);

public:
    LuaExecutor(const Engine::Core::Runtime::ServiceTable* services);
    ~LuaExecutor();

    void Initialize();

    Core::Runtime::CallbackResult ExecuteFile(const char* path);
    Core::Runtime::CallbackResult ExecuteString(const char* string);

    bool LoadScript(const std::vector<unsigned char> *byteCode);
    void ExecuteNode(const InstancedScriptNode &node, Engine::Core::Runtime::EventWriter* writer);
};

}
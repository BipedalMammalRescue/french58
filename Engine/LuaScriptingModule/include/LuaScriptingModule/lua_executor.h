#pragma once

#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/variant.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/event_writer.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Scripting/api_event.h"
#include "EngineCore/Scripting/api_query.h"
#include "lua.h"
#include "LuaScriptingModule/state_data.h"
#include <vector>
namespace Engine::Extension::LuaScriptingModule {

class LuaExecutor
{
private:
    const Engine::Core::Runtime::ServiceTable* m_Services;

    Core::Logging::Logger m_Logger;

    std::vector<InstancedApiQuery> m_ApiQueryList;
    std::vector<InstancedApiEvent> m_ApiEventList;

    std::unordered_map<InstancedScriptParamId, Core::Pipeline::Variant> m_NodeParameters;

    lua_State* m_LuaState = nullptr;

    static int LuaRaiseEvent(lua_State* luaState);
    static int LuaQuery(lua_State* luaState);
    static int L1CallMultiplexer(lua_State* luaState);
    static int LuaPrint(lua_State* luaState);

public:
    LuaExecutor(const Engine::Core::Runtime::ServiceTable* services);
    ~LuaExecutor();

    void Initialize();

    Core::Runtime::CallbackResult ExecuteFile(const char* path);
    Core::Runtime::CallbackResult ExecuteString(const char* string);

    bool LoadScript(void* byteCode, size_t codeLength, int index);
    bool SelectScript(int index);
    void ExecuteNode(const InstancedScriptNode &node, Engine::Core::Runtime::EventWriter* writer);

    Core::Pipeline::Variant GetParameter(const Core::Pipeline::HashId &name, const int &component) const;

    void SetParameter(const Core::Pipeline::HashId &name, const int &component,
                      const Core::Pipeline::Variant &data);
};

}
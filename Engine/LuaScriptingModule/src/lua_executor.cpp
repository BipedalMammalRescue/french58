#include "LuaScriptingModule/lua_executor.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/variant.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/event_writer.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Scripting/api_data.h"
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
#include "EngineCore/Runtime/module_manager.h"
#include "LuaScriptingModule/state_data.h"
#include <md5.h>

using namespace Engine::Extension::LuaScriptingModule;

const char SeScriptTable[] = "SE_SCRIPT_TABLE";
const char SeExecutorInstance[] = "SE_EXECUTOR_INSTANCE";
const char SeApiTable[] = "SE_API_TABLE";
const char SeEventTable[] = "SE_EVENT_TABLE";
const char SeEntityId[] = "SE_COMPONENT_ID";
const char SeScriptParameters[] = "SE_SCRIPT_PARAMETERS";
const char SeEventWriter[] = "SE_EVENT_WRITER";

const char SeGetParameter[] = "GetParameter";
const char SeQuery[] = "EngineQuery";
const char SeEvent[] = "RaiseEvent";

template <typename T>
struct VariantLite
{
    unsigned char Identifier;
    unsigned char IdPad1;
    T Data;
};


template <typename T>
static void WriteVariantLite(lua_State* luaState, const T& src)
{
    auto dest = static_cast<VariantLite<T>*>(lua_newuserdata(luaState, sizeof(VariantLite<T>)));
    *dest = {
        (unsigned char)Engine::Core::Pipeline::ToVariantType<T>(),
        (unsigned char)Engine::Core::Pipeline::ToVariantType<T>(),
        src
    };
}


template <typename T>
static Engine::Core::Scripting::ApiData PopFullUserData(lua_State* luaState, unsigned char identifier, int index)
{
    if (!lua_isuserdata(luaState, index))
        return { Engine::Core::Scripting::ApiDataType::Invalid };

    void* data = lua_touserdata(luaState, index);
    VariantLite<T>* asVariant = static_cast<VariantLite<T>*>(data);

    if (asVariant->Identifier != asVariant->IdPad1 || asVariant->Identifier != identifier)
        return { Engine::Core::Scripting::ApiDataType::Invalid };

    return Engine::Core::Scripting::ToApiData(&asVariant->Data);
}


static bool WriteApiData(lua_State* luaState, Engine::Core::Scripting::ApiDataDefinition type, const Engine::Core::Scripting::ApiData* data)
{
    switch (type.Type)
    {
    case Engine::Core::Scripting::ApiDataType::Object:
        lua_pushlightuserdata(luaState, (void*)data->Data.Object);
        return 1;
    case Engine::Core::Scripting::ApiDataType::Variant:
        switch (type.SubType.Variant)
        {
        case Engine::Core::Pipeline::VariantType::Byte:
            lua_pushinteger(luaState, data->Data.Variant.Data.Byte);
            return 1;
        case Engine::Core::Pipeline::VariantType::Bool:
            lua_pushboolean(luaState, data->Data.Variant.Data.Bool);
            return 1;
        case Engine::Core::Pipeline::VariantType::Int32:
            lua_pushinteger(luaState, data->Data.Variant.Data.Int32);
            return 1;
        case Engine::Core::Pipeline::VariantType::Uint32:
            lua_pushinteger(luaState, data->Data.Variant.Data.Uint32);
            return 1;
        case Engine::Core::Pipeline::VariantType::Float:
            lua_pushnumber(luaState, data->Data.Variant.Data.Float);
            return 1;
        case Engine::Core::Pipeline::VariantType::Vec2:
            WriteVariantLite(luaState, data->Data.Variant.Data.Vec2);
            return 1;
        case Engine::Core::Pipeline::VariantType::Vec3:
            WriteVariantLite(luaState, data->Data.Variant.Data.Vec3);
            return 1;
        case Engine::Core::Pipeline::VariantType::Vec4:
            WriteVariantLite(luaState, data->Data.Variant.Data.Vec4);
            return 1;
        case Engine::Core::Pipeline::VariantType::Mat2:
            WriteVariantLite(luaState, data->Data.Variant.Data.Mat2);
            return 1;
        case Engine::Core::Pipeline::VariantType::Mat3:
            WriteVariantLite(luaState, data->Data.Variant.Data.Mat3);
            return 1;
        case Engine::Core::Pipeline::VariantType::Mat4:
            WriteVariantLite(luaState, data->Data.Variant.Data.Mat4);
            return 1;
        case Engine::Core::Pipeline::VariantType::Path:
            WriteVariantLite(luaState, data->Data.Variant.Data.Path);
            return 1;
        default:
            return 0;
        }
    default:
        return 0;
    }
}


static Engine::Core::Scripting::ApiData ReadApiData(lua_State* luaState, Engine::Core::Scripting::ApiDataDefinition type, int index)
{
    switch (type.Type)
    {
    case Engine::Core::Scripting::ApiDataType::Object:
        if (!lua_islightuserdata(luaState, index))
            return { Engine::Core::Scripting::ApiDataType::Invalid };
        return { .Type = Engine::Core::Scripting::ApiDataType::Object, .Data { .Object = lua_touserdata(luaState, index) } };
    case Engine::Core::Scripting::ApiDataType::Variant:
        switch (type.SubType.Variant)
        {
        case Engine::Core::Pipeline::VariantType::Byte:
            {
                if (!lua_isinteger(luaState, index))
                    return { Engine::Core::Scripting::ApiDataType::Invalid };

                int tempInteger = lua_tointeger(luaState, index);
                if (tempInteger < 0 || tempInteger > 0xFF)
                    return { Engine::Core::Scripting::ApiDataType::Invalid };

                Engine::Core::Scripting::ApiData result { Engine::Core::Scripting::ApiDataType::Variant };
                result.Data.Variant.Type = Engine::Core::Pipeline::VariantType::Byte;
                result.Data.Variant.Data.Byte = tempInteger;
                return result;
            }
        case Engine::Core::Pipeline::VariantType::Bool:
            {
                if (!lua_isboolean(luaState, index))
                    return { Engine::Core::Scripting::ApiDataType::Invalid };
                Engine::Core::Scripting::ApiData result { Engine::Core::Scripting::ApiDataType::Variant };
                result.Data.Variant.Type = Engine::Core::Pipeline::VariantType::Bool;
                result.Data.Variant.Data.Bool = lua_toboolean(luaState, index);
                return result;
            }
        case Engine::Core::Pipeline::VariantType::Int32:
            {
                if (!lua_isinteger(luaState, index))
                    return { Engine::Core::Scripting::ApiDataType::Invalid };
                Engine::Core::Scripting::ApiData result { Engine::Core::Scripting::ApiDataType::Variant };
                result.Data.Variant.Type = Engine::Core::Pipeline::VariantType::Int32;
                result.Data.Variant.Data.Int32 = lua_tointeger(luaState, index);
                return result;
            }
        case Engine::Core::Pipeline::VariantType::Uint32:
            {
                if (!lua_isinteger(luaState, index))
                    return { Engine::Core::Scripting::ApiDataType::Invalid };

                int tempInteger = lua_tointeger(luaState, index);
                if (tempInteger < 0)
                    return { Engine::Core::Scripting::ApiDataType::Invalid };

                Engine::Core::Scripting::ApiData result { Engine::Core::Scripting::ApiDataType::Variant };
                result.Data.Variant.Type = Engine::Core::Pipeline::VariantType::Uint32;
                result.Data.Variant.Data.Uint32 = tempInteger;
                return result;
            }
        case Engine::Core::Pipeline::VariantType::Float:
            {
                if (!lua_isinteger(luaState, index))
                    return { Engine::Core::Scripting::ApiDataType::Invalid };
                Engine::Core::Scripting::ApiData result { Engine::Core::Scripting::ApiDataType::Variant };
                result.Data.Variant.Type = Engine::Core::Pipeline::VariantType::Byte;
                result.Data.Variant.Data.Byte = lua_tointeger(luaState, index);
                return result;
            }

        case Engine::Core::Pipeline::VariantType::Vec2:
            return PopFullUserData<glm::vec2>(luaState, (unsigned char)type.SubType.Variant, index);
        case Engine::Core::Pipeline::VariantType::Vec3:
            return PopFullUserData<glm::vec3>(luaState, (unsigned char)type.SubType.Variant, index);
        case Engine::Core::Pipeline::VariantType::Vec4:
            return PopFullUserData<glm::vec4>(luaState, (unsigned char)type.SubType.Variant, index);
        case Engine::Core::Pipeline::VariantType::Mat2:
            return PopFullUserData<glm::mat2>(luaState, (unsigned char)type.SubType.Variant, index);
        case Engine::Core::Pipeline::VariantType::Mat3:
            return PopFullUserData<glm::mat3>(luaState, (unsigned char)type.SubType.Variant, index);
        case Engine::Core::Pipeline::VariantType::Mat4:
            return PopFullUserData<glm::mat4>(luaState, (unsigned char)type.SubType.Variant, index);
        case Engine::Core::Pipeline::VariantType::Path:
            return PopFullUserData<Engine::Core::Pipeline::HashId>(luaState, (unsigned char)type.SubType.Variant, index);
        default:
            return { Engine::Core::Scripting::ApiDataType::Invalid };
        }
    default:
        return { Engine::Core::Scripting::ApiDataType::Invalid };
    }
}


static int CreateVec2(lua_State* luaState)
{
    if (!lua_isnumber(luaState, -1) 
        || !lua_isnumber(luaState, -2))
        return 0;

    float x = lua_tonumber(luaState, -2);
    float y = lua_tonumber(luaState, -1);

    WriteVariantLite(luaState, glm::vec2(x, y));
    return 1;
}

static int CreateVec3(lua_State* luaState)
{
    if (!lua_isnumber(luaState, -1) 
        || !lua_isnumber(luaState, -2)
        || !lua_isnumber(luaState, -3))
        return 0;

    float x = lua_tonumber(luaState, -3);
    float y = lua_tonumber(luaState, -2);
    float z = lua_tonumber(luaState, -1);

    WriteVariantLite(luaState, glm::vec3(x, y, z));
    return 1;
}

static int CreateVec4(lua_State* luaState)
{
    if (!lua_isnumber(luaState, -1) 
        || !lua_isnumber(luaState, -2)
        || !lua_isnumber(luaState, -3)
        || !lua_isnumber(luaState, -4))
        return 0;

    float x = lua_tonumber(luaState, -4);
    float y = lua_tonumber(luaState, -3);
    float z = lua_tonumber(luaState, -2);
    float w = lua_tonumber(luaState, -1);

    WriteVariantLite(luaState, glm::vec4(x, y, z, w));
    return 1;
}


static int GetLuaScriptParameter(lua_State* luaState)
{
    if (!lua_isstring(luaState, -1))
        return 0;

    const char* paramName = lua_tostring(luaState, -1);
    Engine::Core::Pipeline::HashId paramId = md5::compute(paramName);

    lua_getglobal(luaState, SeScriptParameters);
    if (!lua_isuserdata(luaState, -1))
        return 0;

    auto parameters = static_cast<std::unordered_map<Engine::Core::Pipeline::HashId, Engine::Core::Pipeline::Variant>*>(lua_touserdata(luaState, -1));
    auto foundParam = parameters->find(paramId);

    if (foundParam == parameters->end())
        return 0;

    Engine::Core::Scripting::ApiData data;
    data.Type = Engine::Core::Scripting::ApiDataType::Variant;
    data.Data.Variant = foundParam->second;

    return WriteApiData(luaState, 
        { .Type = Engine::Core::Scripting::ApiDataType::Variant, .SubType = { .Variant = foundParam->second.Type } }, 
        &data) ? 1 : 0;
}


int LuaExecutor::LuaRaiseEvent(lua_State* luaState)
{
    int offset = 0;

    int stackDepth = lua_gettop(luaState);
    if (!lua_isinteger(luaState, 0 - stackDepth))
        return 0;
    int eventIndex = lua_tointeger(luaState, 0 - stackDepth);

    lua_getglobal(luaState, SeExecutorInstance);
    offset --;
    auto executor = static_cast<LuaExecutor*>(lua_touserdata(luaState, -1));

    lua_getglobal(luaState, SeEventWriter);
    offset --;
    auto writer = static_cast<Core::Runtime::EventWriter*>(lua_touserdata(luaState, -1));

    if (eventIndex < 0 || eventIndex >= executor->m_ApiEventList.size())
        return 0;

    auto event = executor->m_ApiEventList[eventIndex];
    if (stackDepth - 1 != event.Api->GetParamCount())
        return 0;

    switch (event.Api->GetParamCount())
    {
    case 0:
        {
            auto apiActual = static_cast<const Core::Scripting::ApiEvent_0*>(event.Api);
            apiActual->Run(executor->m_Services, event.ModuleState, writer, 0);
        }
        break;
    case 1:
        {
            auto apiActual = static_cast<const Core::Scripting::ApiEventBase_1*>(event.Api);

            Engine::Core::Scripting::ApiData p1 = ReadApiData(luaState, apiActual->GetP1Type(), offset - 1);
            
            apiActual->Run(executor->m_Services, event.ModuleState, p1, writer, 0);
        }
        break;
    case 2:
        {
            auto apiActual = static_cast<const Core::Scripting::ApiEventBase_2*>(event.Api);

            Engine::Core::Scripting::ApiData p2 = ReadApiData(luaState, apiActual->GetP2Type(), offset - 1);
            Engine::Core::Scripting::ApiData p1 = ReadApiData(luaState, apiActual->GetP1Type(), offset - 2);
            
            apiActual->Run(executor->m_Services, event.ModuleState, p1, p2, writer, 0);
        }
        break;
    case 3:
        {
            auto apiActual = static_cast<const Core::Scripting::ApiEventBase_3*>(event.Api);

            Engine::Core::Scripting::ApiData p3 = ReadApiData(luaState, apiActual->GetP3Type(), offset - 1);
            Engine::Core::Scripting::ApiData p2 = ReadApiData(luaState, apiActual->GetP2Type(), offset - 2);
            Engine::Core::Scripting::ApiData p1 = ReadApiData(luaState, apiActual->GetP1Type(), offset - 3);
            
            apiActual->Run(executor->m_Services, event.ModuleState, p1, p2, p3, writer, 0);
        }
        break;
    case 4:
        {
            auto apiActual = static_cast<const Core::Scripting::ApiEventBase_4*>(event.Api);

            Engine::Core::Scripting::ApiData p4 = ReadApiData(luaState, apiActual->GetP4Type(), offset - 1);
            Engine::Core::Scripting::ApiData p3 = ReadApiData(luaState, apiActual->GetP3Type(), offset - 2);
            Engine::Core::Scripting::ApiData p2 = ReadApiData(luaState, apiActual->GetP2Type(), offset - 3);
            Engine::Core::Scripting::ApiData p1 = ReadApiData(luaState, apiActual->GetP1Type(), offset - 4);
            
            apiActual->Run(executor->m_Services, event.ModuleState, p1, p2, p3, p4, writer, 0);
        }
        break;
    }

    return 0;
}

int LuaExecutor::LuaQuery(lua_State* luaState)
{
    int offset = 0;

    int stackDepth = lua_gettop(luaState);
    if (!lua_isinteger(luaState, 0 - stackDepth))
        return 0;
    int apiIndex = lua_tointeger(luaState, 0 - stackDepth);

    lua_getglobal(luaState, SeExecutorInstance);
    offset --;
    auto executor = static_cast<LuaExecutor*>(lua_touserdata(luaState, -1));

    if (apiIndex < 0 || apiIndex >= executor->m_ApiQueryList.size())
        return 0;

    auto api = executor->m_ApiQueryList[apiIndex];

    if (stackDepth - 1 != api.Api->GetParamCount())
        return 0;

    switch (api.Api->GetParamCount())
    {
    case 0:
        {
            auto realApi = static_cast<const Engine::Core::Scripting::ApiQueryBase_0*>(api.Api);

            // run
            Engine::Core::Scripting::ApiData result = realApi->Run(executor->m_Services, api.ModuleState);

            // write the result
            return WriteApiData(luaState, realApi->GetReturnType(), &result) ? 1 : 0;
        }
    case 1:
        {
            auto realApi = static_cast<const Engine::Core::Scripting::ApiQueryBase_1*>(api.Api);

            // run
            Engine::Core::Scripting::ApiData p1 = ReadApiData(luaState, realApi->GetP1Type(), offset - 1);
            Engine::Core::Scripting::ApiData result = realApi->Run(executor->m_Services, api.ModuleState, p1);

            // write the result
            return WriteApiData(luaState, realApi->GetReturnType(), &result) ? 1 : 0;
        }
    default:
        return 0;
    }
}

Engine::Core::Runtime::CallbackResult LuaExecutor::ExecuteFile(const char* path)
{
    if (luaL_dofile(m_LuaState, path) != LUA_OK)
    {
        std::string error(lua_isstring(m_LuaState, -1) ? lua_tostring(m_LuaState, -1) : "unknown error");
        return Core::Runtime::Crash(__FILE__, __LINE__, error);
    }
    return Core::Runtime::CallbackSuccess();
}

Engine::Core::Runtime::CallbackResult LuaExecutor::ExecuteString(const char* string)
{
    if (luaL_dostring(m_LuaState, string) != LUA_OK)
    {
        std::string error(lua_isstring(m_LuaState, -1) ? lua_tostring(m_LuaState, -1) : "unknown error");
        return Core::Runtime::Crash(__FILE__, __LINE__, error);
    }
    return Core::Runtime::CallbackSuccess();
}

void LuaExecutor::Initialize()
{
    // insert self into the table (this method needs to be called everytime the object moves)
    lua_pushlightuserdata(m_LuaState, this);
    lua_setglobal(m_LuaState, SeExecutorInstance);

    // get the modules
    Engine::Core::Pipeline::ModuleAssembly modules = Engine::Core::Pipeline::ListModules();

    // set up the api table
    lua_createtable(m_LuaState, 0, modules.ModuleCount);
    for (size_t i = 0; i < modules.ModuleCount; i++)
    {
        if (modules.Modules[i].ApiQueryCount <= 0)
            continue;

        lua_pushstring(m_LuaState, modules.Modules[i].Name.DisplayName);
        lua_createtable(m_LuaState, 0, modules.Modules[i].ApiQueryCount);
        for (size_t j = 0; j < modules.Modules[i].ApiQueryCount; j++)
        {
            auto api = modules.Modules[i].ApiQueries[j];

            // find module state (skip if not found)
            const void* moduleState = m_Services->ModuleManager->FindModule(modules.Modules[i].Name.Hash);
            if (moduleState == nullptr)
                continue;

            // insert the instanced api
            InstancedApiQuery instancedApi {moduleState, api};
            m_ApiQueryList.push_back(instancedApi);

            // register it in the inner api table
            lua_pushstring(m_LuaState, api->GetName());
            lua_pushinteger(m_LuaState, m_ApiQueryList.size() - 1);
            lua_settable(m_LuaState, -3);
        }
        lua_settable(m_LuaState, -3);
    }
    lua_setglobal(m_LuaState, SeApiTable);

    // set up the event table
    lua_createtable(m_LuaState, 0, modules.ModuleCount);
    for (size_t i = 0; i < modules.ModuleCount; i++)
    {
        if (modules.Modules[i].ApiEventCount <= 0)
            continue;

        lua_pushstring(m_LuaState, modules.Modules[i].Name.DisplayName);
        lua_createtable(m_LuaState, 0, modules.Modules[i].ApiEventCount);
        for (size_t j = 0; j < modules.Modules[i].ApiEventCount; j++)
        {
            auto api = modules.Modules[i].ApiEvents[j];

            // find module state (skip if not found)
            const void* moduleState = m_Services->ModuleManager->FindModule(modules.Modules[i].Name.Hash);
            if (moduleState == nullptr)
                continue;

            // insert the instanced api
            InstancedApiEvent instancedApi {moduleState, api};
            m_ApiEventList.push_back(instancedApi);

            // register it in the inner api table
            lua_pushstring(m_LuaState, api->GetName());
            lua_pushinteger(m_LuaState, m_ApiEventList.size() - 1);
            lua_settable(m_LuaState, -3);
        }
        lua_settable(m_LuaState, -3);
    }
    lua_setglobal(m_LuaState, SeEventTable);

    // script table (not sure how many scritps are there)
    lua_createtable(m_LuaState, 0, 0);
    lua_setglobal(m_LuaState, SeScriptTable);

    // install the invoke function
    lua_pushcfunction(m_LuaState, LuaQuery);
    lua_setglobal(m_LuaState, SeQuery);

    // functions that's used for raising events
    lua_pushcfunction(m_LuaState, LuaRaiseEvent);
    lua_setglobal(m_LuaState, SeEvent);

    // used for parameters
    lua_pushcfunction(m_LuaState, GetLuaScriptParameter);
    lua_setglobal(m_LuaState, SeGetParameter);

    // library functions
    lua_pushcfunction(m_LuaState, CreateVec2);
    lua_setglobal(m_LuaState, "vec2");

    lua_pushcfunction(m_LuaState, CreateVec3);
    lua_setglobal(m_LuaState, "vec3");

    lua_pushcfunction(m_LuaState, CreateVec4);
    lua_setglobal(m_LuaState, "vec4");

    // TODO: matrix and multiplication and stuff

    m_Logger.Information("Lua executor initialized.");
}

LuaExecutor::LuaExecutor(const Engine::Core::Runtime::ServiceTable* services) : m_Services(services)
{
    static const char* LogChannel[] = { "LuaExecutor" };
    m_Logger = services->LoggerService->CreateLogger(LogChannel, 1);

    m_LuaState = luaL_newstate();
    luaL_openlibs(m_LuaState);
}

LuaExecutor::~LuaExecutor() 
{
    lua_close(m_LuaState);
}

void Engine::Extension::LuaScriptingModule::LuaExecutor::ExecuteNode(const InstancedScriptNode &node, Engine::Core::Runtime::EventWriter* writer) 
{
    if (!lua_isfunction(m_LuaState, -1))
        return;

    Core::Runtime::EventWriterCheckpoint checkpoint = writer->CreateCheckpoint();

    lua_pushinteger(m_LuaState, node.Entity);
    lua_setglobal(m_LuaState, SeEntityId);

    lua_pushlightuserdata(m_LuaState, (void*)&node.Parameters);
    lua_setglobal(m_LuaState, SeScriptParameters);

    lua_pushlightuserdata(m_LuaState, writer);
    lua_setglobal(m_LuaState, SeEventWriter);

    auto result = lua_pcall(m_LuaState, 0, 0, 0);
    if (result != LUA_OK)
    {
        // TODO: this is a hack, it's not memory safe: we need a real string passing mechanism
        writer->Rollback(checkpoint);
        m_Logger.Error("Lua script execution failed, return code: {return}, error: {error}", { result, lua_tostring(m_LuaState, -1) });
    }
}

bool Engine::Extension::LuaScriptingModule::LuaExecutor::LoadScript(const std::vector<unsigned char> *byteCode, int index) 
{
    lua_getglobal(m_LuaState, SeScriptTable);
    lua_pushinteger(m_LuaState, index);

    auto result = luaL_loadbuffer(m_LuaState, (const char*)byteCode->data(), byteCode->size(), "");
    if (result != LUA_OK)
        return false;
    
    lua_settable(m_LuaState, -3);
    return true;
}

bool LuaExecutor::SelectScript(int index)
{
    lua_getglobal(m_LuaState, SeScriptTable);
    lua_rawgeti(m_LuaState, -1, index);
    return lua_isfunction(m_LuaState, -1);
}
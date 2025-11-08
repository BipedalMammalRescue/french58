#include "LuaScriptingModule/lua_executor.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/service_table.h"
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
#include "EngineCore/Runtime/module_manager.h"
#include <string>

using namespace Engine::Extension::LuaScriptingModule;

const char SeExecutorInstance[] = "SE_EXECUTOR_INSTANCE";
const char SeApiTable[] = "SE_API_TABLE";
const char SeFindApi[] = "SE_FIND_API";
const char SeInvokeApi[] = "SE_INVOKE_API";

template <typename T>
struct VariantLite
{
    unsigned char Identifier;
    unsigned char IdPad1;
    T Data;
};

template <typename T>
static void PushFullUserdata(lua_State* luaState, const T* data, unsigned char identifier)
{
    void* dest = lua_newuserdata(luaState, sizeof(VariantLite<T>));
    *static_cast<VariantLite<T>*>(dest) = { identifier, identifier, *data };
}


template <typename T>
static Engine::Core::Scripting::ApiData PopFullUserData(lua_State* luaState, unsigned char identifier)
{
    if (!lua_isuserdata(luaState, -1))
        return { Engine::Core::Scripting::ApiDataType::Invalid };

    void* data = lua_touserdata(luaState, -1);
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
        switch (type.SubType)
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
            PushFullUserdata(luaState, &data->Data.Variant.Data.Vec2, (unsigned char)Engine::Core::Pipeline::VariantType::Vec2);
            return 1;
        case Engine::Core::Pipeline::VariantType::Vec3:
            PushFullUserdata(luaState, &data->Data.Variant.Data.Vec3, (unsigned char)Engine::Core::Pipeline::VariantType::Vec3);
            return 1;
        case Engine::Core::Pipeline::VariantType::Vec4:
            PushFullUserdata(luaState, &data->Data.Variant.Data.Vec4, (unsigned char)Engine::Core::Pipeline::VariantType::Vec4);
            return 1;
        case Engine::Core::Pipeline::VariantType::Mat2:
            PushFullUserdata(luaState, &data->Data.Variant.Data.Mat2, (unsigned char)Engine::Core::Pipeline::VariantType::Mat2);
            return 1;
        case Engine::Core::Pipeline::VariantType::Mat3:
            PushFullUserdata(luaState, &data->Data.Variant.Data.Mat3, (unsigned char)Engine::Core::Pipeline::VariantType::Mat3);
            return 1;
        case Engine::Core::Pipeline::VariantType::Mat4:
            PushFullUserdata(luaState, &data->Data.Variant.Data.Mat4, (unsigned char)Engine::Core::Pipeline::VariantType::Mat4);
            return 1;
        case Engine::Core::Pipeline::VariantType::Path:
            PushFullUserdata(luaState, &data->Data.Variant.Data.Path, (unsigned char)Engine::Core::Pipeline::VariantType::Path);
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
        switch (type.SubType)
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
            return PopFullUserData<glm::vec2>(luaState, (unsigned char)type.SubType);
        case Engine::Core::Pipeline::VariantType::Vec3:
            return PopFullUserData<glm::vec3>(luaState, (unsigned char)type.SubType);
        case Engine::Core::Pipeline::VariantType::Vec4:
            return PopFullUserData<glm::vec4>(luaState, (unsigned char)type.SubType);
        case Engine::Core::Pipeline::VariantType::Mat2:
            return PopFullUserData<glm::mat2>(luaState, (unsigned char)type.SubType);
        case Engine::Core::Pipeline::VariantType::Mat3:
            return PopFullUserData<glm::mat3>(luaState, (unsigned char)type.SubType);
        case Engine::Core::Pipeline::VariantType::Mat4:
            return PopFullUserData<glm::mat4>(luaState, (unsigned char)type.SubType);
        case Engine::Core::Pipeline::VariantType::Path:
            return PopFullUserData<Engine::Core::Pipeline::HashId>(luaState, (unsigned char)type.SubType);
        default:
            return { Engine::Core::Scripting::ApiDataType::Invalid };
        }
    default:
        return { Engine::Core::Scripting::ApiDataType::Invalid };
    }
}


int LuaExecutor::LuaInvoke(lua_State* luaState)
{
    int offset = 0;

    int stackDepth = lua_gettop(luaState);
    if (!lua_isinteger(luaState, 0 - stackDepth))
        return 0;
    int apiIndex = lua_tointeger(luaState, 0 - stackDepth);

    lua_getglobal(luaState, SeExecutorInstance);
    offset --;
    auto executor = static_cast<LuaExecutor*>(lua_touserdata(luaState, -1));

    if (apiIndex < 0 || apiIndex >= executor->m_ApiList.size())
        return 0;

    auto api = executor->m_ApiList[apiIndex];

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

int LuaExecutor::L1CallMultiplexer(lua_State* luaState)
{
    if (!lua_isstring(luaState, -1) || !lua_isstring(luaState, -2))
        return 0;

    const char* moduleName = lua_tostring(luaState, -2);
    const char* apiName = lua_tostring(luaState, -1);

    lua_getglobal(luaState, "SE_API_TABLE");
    lua_getfield(luaState, -1, moduleName);
    lua_getfield(luaState, -1, apiName);

    if (!lua_isinteger(luaState, -1))
        return 0;
    
    return 1;
}

Engine::Core::Runtime::CallbackResult LuaExecutor::ExecuteFile(const char* path)
{
    if (!luaL_dofile(m_LuaState, path))
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

    // set up the api table
    Engine::Core::Pipeline::ModuleAssembly modules = Engine::Core::Pipeline::ListModules();
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
            InstancedApi instancedApi {moduleState, api};
            m_ApiList.push_back(instancedApi);

            // register it in the inner api table
            lua_pushstring(m_LuaState, api->GetName());
            lua_pushinteger(m_LuaState, m_ApiList.size() - 1);
            lua_settable(m_LuaState, -3);
        }
        lua_settable(m_LuaState, -3);
    }
    lua_setglobal(m_LuaState, SeApiTable);

    // install the invoke function
    lua_pushcfunction(m_LuaState, L1CallMultiplexer);
    lua_setglobal(m_LuaState, SeFindApi);
    lua_pushcfunction(m_LuaState, LuaInvoke);
    lua_setglobal(m_LuaState, SeInvokeApi);

    // TODO: library functions

    // TODO: raise events
}

LuaExecutor::LuaExecutor(const Engine::Core::Runtime::ServiceTable* services) : m_Services(services)
{
    m_LuaState = luaL_newstate();
    luaL_openlibs(m_LuaState);
}

LuaExecutor::~LuaExecutor()
{
    lua_close(m_LuaState);
}
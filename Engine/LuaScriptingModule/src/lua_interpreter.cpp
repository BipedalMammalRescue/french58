#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/variant.h"
#include "EngineCore/Scripting/api_data.h"
#include "EngineCore/Scripting/api_query.h"

#include "glm/ext/matrix_float3x3.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float2.hpp"
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
#include <cstdio>
#include <lua.hpp>
#include <iostream>


class SampleQuery : public Engine::Core::Scripting::ApiQuery_0<int>
{
protected:
    Engine::Core::Scripting::ReturnContainer<int> RunCore(const Engine::Core::Runtime::ServiceTable* services, const void* moduleState) const override 
    {
        return { 42 };
    }

public:
    const char* GetName() const override 
    {
        return "GetMagicNumber";
    }

    static const Engine::Core::Scripting::ApiQueryBase* GetQuery()
    {
        static const SampleQuery query;
        return &query;
    }
};


static const Engine::Core::Scripting::ApiQueryBase* ApiTable[] { SampleQuery::GetQuery() };
size_t ApiTableLength = sizeof(ApiTable) / sizeof(Engine::Core::Scripting::ApiQueryBase*);


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


static Engine::Core::Scripting::ApiData ReadApiData(lua_State* luaState, Engine::Core::Scripting::ApiDataDefinition type)
{
    switch (type.Type)
    {
    case Engine::Core::Scripting::ApiDataType::Object:
        if (!lua_islightuserdata(luaState, -1))
            return { Engine::Core::Scripting::ApiDataType::Invalid };
        return { .Type = Engine::Core::Scripting::ApiDataType::Object, .Data { .Object = lua_touserdata(luaState, -1) } };
    case Engine::Core::Scripting::ApiDataType::Variant:
        switch (type.SubType)
        {
        case Engine::Core::Pipeline::VariantType::Byte:
            {
                if (!lua_isinteger(luaState, -1))
                    return { Engine::Core::Scripting::ApiDataType::Invalid };

                int tempInteger = lua_tointeger(luaState, -1);
                if (tempInteger < 0 || tempInteger > 0xFF)
                    return { Engine::Core::Scripting::ApiDataType::Invalid };

                Engine::Core::Scripting::ApiData result { Engine::Core::Scripting::ApiDataType::Variant };
                result.Data.Variant.Type = Engine::Core::Pipeline::VariantType::Byte;
                result.Data.Variant.Data.Byte = tempInteger;
                return result;
            }
        case Engine::Core::Pipeline::VariantType::Bool:
            {
                if (!lua_isboolean(luaState, -1))
                    return { Engine::Core::Scripting::ApiDataType::Invalid };
                Engine::Core::Scripting::ApiData result { Engine::Core::Scripting::ApiDataType::Variant };
                result.Data.Variant.Type = Engine::Core::Pipeline::VariantType::Bool;
                result.Data.Variant.Data.Bool = lua_toboolean(luaState, -1);
                return result;
            }
        case Engine::Core::Pipeline::VariantType::Int32:
            {
                if (!lua_isinteger(luaState, -1))
                    return { Engine::Core::Scripting::ApiDataType::Invalid };
                Engine::Core::Scripting::ApiData result { Engine::Core::Scripting::ApiDataType::Variant };
                result.Data.Variant.Type = Engine::Core::Pipeline::VariantType::Int32;
                result.Data.Variant.Data.Int32 = lua_tointeger(luaState, -1);
                return result;
            }
        case Engine::Core::Pipeline::VariantType::Uint32:
            {
                if (!lua_isinteger(luaState, -1))
                    return { Engine::Core::Scripting::ApiDataType::Invalid };

                int tempInteger = lua_tointeger(luaState, -1);
                if (tempInteger < 0)
                    return { Engine::Core::Scripting::ApiDataType::Invalid };

                Engine::Core::Scripting::ApiData result { Engine::Core::Scripting::ApiDataType::Variant };
                result.Data.Variant.Type = Engine::Core::Pipeline::VariantType::Uint32;
                result.Data.Variant.Data.Uint32 = tempInteger;
                return result;
            }
        case Engine::Core::Pipeline::VariantType::Float:
            {
                if (!lua_isinteger(luaState, -1))
                    return { Engine::Core::Scripting::ApiDataType::Invalid };
                Engine::Core::Scripting::ApiData result { Engine::Core::Scripting::ApiDataType::Variant };
                result.Data.Variant.Type = Engine::Core::Pipeline::VariantType::Byte;
                result.Data.Variant.Data.Byte = lua_tointeger(luaState, -1);
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



static int LuaInvoke(lua_State* luaState)
{
    lua_getglobal(luaState, "SE_API_GLOBAL_INDEX");
    int apiIndex = lua_tointeger(luaState, -1);

    if (apiIndex < 0 || apiIndex >= ApiTableLength)
        return 0;

    const Engine::Core::Scripting::ApiQueryBase* api = ApiTable[apiIndex];

    switch (api->GetParamCount())
    {
    case 0:
        {
            auto realApi = static_cast<const Engine::Core::Scripting::ApiQueryBase_0*>(api);

            // run
            Engine::Core::Scripting::ApiData result = realApi->Run(nullptr, nullptr);

            // write the result
            return WriteApiData(luaState, realApi->GetReturnType(), &result) ? 1 : 0;
        }
    case 1:
        {
            auto realApi = static_cast<const Engine::Core::Scripting::ApiQueryBase_1*>(api);

            // run
            Engine::Core::Scripting::ApiData p1 = ReadApiData(luaState, realApi->GetP1Type());
            Engine::Core::Scripting::ApiData result = realApi->Run(nullptr, nullptr, p1);

            // write the result
            return WriteApiData(luaState, realApi->GetReturnType(), &result) ? 1 : 0;
        }
    default:
        return 0;
    }
}


static int LuaCallMultiplexer(lua_State* luaState)
{
    if (!lua_isstring(luaState, -1))
        return 0;

    const char* key = lua_tostring(luaState, -1);
    lua_getglobal(luaState, "SE_API_TABLE");
    lua_getfield(luaState, -1, key);

    if (!lua_isinteger(luaState, -1))
        return 0;
        
    lua_setglobal(luaState, "SE_API_GLOBAL_INDEX");
    lua_pushcfunction(luaState, LuaInvoke);
    return 1;
}


int main()
{
    // set up the runtime
    lua_State* luaState = luaL_newstate();
    luaL_openlibs(luaState);

    // set up the table
    lua_createtable(luaState, 0, ApiTableLength);
    for (size_t i = 0; i < ApiTableLength; i++)
    {
        lua_pushstring(luaState, ApiTable[i]->GetName());
        lua_pushinteger(luaState, i);
        lua_settable(luaState, -3);
    }
    lua_setglobal(luaState, "SE_API_TABLE");

    // set up the api access proxy
    lua_createtable(luaState, 0, 0);
    lua_createtable(luaState, 0, 1);
    lua_pushstring(luaState, "__index");
    lua_pushcfunction(luaState, LuaCallMultiplexer);
    lua_settable(luaState, -3);
    lua_setmetatable(luaState, -2);
    lua_setglobal(luaState, "SE_ENGINE_API");

    // execute
    if (!luaL_dofile(luaState, "test.lua") == LUA_OK) 
    {
        std::cout << "[C] Error reading script\n";
        printf("Error: %s\n", lua_tostring(luaState, -1));
    }

    lua_close(luaState);
}
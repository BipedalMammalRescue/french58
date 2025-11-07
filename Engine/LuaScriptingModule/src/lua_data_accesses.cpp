#include "LuaScriptingModule/lua_data_accesses.h"

#include "EngineCore/Scripting/script_object.h"
#include "EngineCore/Scripting/script_property.h"
#include "lua.h"
#include <cstddef>

template <typename T>
static size_t GetUserData(lua_State* luaState, void* data, int index)
{
    T* userdata = static_cast<T*>(lua_touserdata(luaState, index));
    *static_cast<T*>(data) = *userdata;
    return sizeof(T);
}

template <typename T>
static size_t PushUserData(lua_State* luaState, const void* data)
{
    T *newData = static_cast<T *>(lua_newuserdata(luaState, sizeof(T)));
    *newData = *static_cast<const T*>(data);
    return sizeof(T);
}

static size_t Align(size_t raw, size_t alignment)
{
    raw += alignment - 1;
    raw -= raw % alignment;
    return raw;
}

// TODO: need to check type

size_t Engine::Extension::LuaScriptingModule::PopScriptObject(const Engine::Core::Scripting::ScriptObject* reflection, void* data, lua_State* luaState, int index)
{
    switch (reflection->ObjectType)
    {
    case Engine::Core::Scripting::ScriptObjectType::Void:
        return 0;
    case Engine::Core::Scripting::ScriptObjectType::Unary:
        {
            switch (reflection->DataType.UnaryType)
            {
            case Engine::Core::Pipeline::VariantType::Byte:
                *static_cast<unsigned char*>(data) = static_cast<unsigned char>(lua_tointeger(luaState, index));
                return sizeof(unsigned char);
            case Engine::Core::Pipeline::VariantType::Bool:
                *static_cast<bool*>(data) = static_cast<bool>(lua_toboolean(luaState, index));
                return sizeof(bool);
            case Engine::Core::Pipeline::VariantType::Int32:
                *static_cast<int*>(data) = static_cast<int>(lua_tointeger(luaState, index));
                return sizeof(int);
            case Engine::Core::Pipeline::VariantType::Uint32:
                *static_cast<unsigned int*>(data) = static_cast<unsigned int>(lua_tointeger(luaState, index));
                return sizeof(unsigned int);
            case Engine::Core::Pipeline::VariantType::Float:
                *static_cast<float*>(data) = static_cast<float>(lua_tonumber(luaState, index));
                return sizeof(float);
            case Engine::Core::Pipeline::VariantType::Vec2:
                return GetUserData<glm::vec2>(luaState, data, index);
            case Engine::Core::Pipeline::VariantType::Vec3:
                return GetUserData<glm::vec3>(luaState, data, index);
            case Engine::Core::Pipeline::VariantType::Vec4:
                return GetUserData<glm::vec4>(luaState, data, index);
            case Engine::Core::Pipeline::VariantType::Mat2:
                return GetUserData<glm::mat2>(luaState, data, index);
            case Engine::Core::Pipeline::VariantType::Mat3:
                return GetUserData<glm::mat3>(luaState, data, index);
            case Engine::Core::Pipeline::VariantType::Mat4:
                return GetUserData<glm::mat4>(luaState, data, index);
            case Engine::Core::Pipeline::VariantType::Path:
                return GetUserData<Engine::Core::Pipeline::HashId>(luaState, data, index);
            case Engine::Core::Pipeline::VariantType::Invalid:
                return 0;
            }
        }
    case Engine::Core::Scripting::ScriptObjectType::Struct:
        {
            if (!lua_istable(luaState, index))
                return 0;

            size_t writeCounter = 0;
            int stackDepth = 0;
            
            for (size_t i = 0; i < reflection->DataType.StructType.PropertyCount; i++)
            {
                writeCounter = Align(writeCounter, reflection->DataType.StructType.Properties[i].Type->Alignment);

                lua_getfield(luaState, index - stackDepth, reflection->DataType.StructType.Properties[i].Name);
                size_t newWrite = PopScriptObject(reflection->DataType.StructType.Properties[i].Type, static_cast<unsigned char*>(data) + writeCounter, luaState, -1);
                stackDepth ++;
                writeCounter += newWrite;
            }

            return writeCounter;
        }
    case Engine::Core::Scripting::ScriptObjectType::Array:
        // TODO: pass array from lua to c++ not supported yet
        {
            struct ArrayHeader
            {
                const void* Data;
                size_t Length;
            };
            ArrayHeader* header = static_cast<ArrayHeader*>(data);
            header->Data = nullptr;
            header->Length = 0;
            return sizeof(ArrayHeader);
        }
    default:
        return 0;
    }
}

size_t Engine::Extension::LuaScriptingModule::PushScriptObject(const Engine::Core::Scripting::ScriptObject* reflection, const void* data, lua_State* luaState)
{
    switch (reflection->ObjectType)
    {
    case Engine::Core::Scripting::ScriptObjectType::Void:
        return 0;
    case Engine::Core::Scripting::ScriptObjectType::Unary:
        {
            switch (reflection->DataType.UnaryType)
            {
            case Engine::Core::Pipeline::VariantType::Byte:
                lua_pushinteger(luaState, *static_cast<const unsigned char*>(data));
                return sizeof(unsigned char);
            case Engine::Core::Pipeline::VariantType::Bool:
                lua_pushboolean(luaState, *static_cast<const bool*>(data));
                return sizeof(bool);
            case Engine::Core::Pipeline::VariantType::Int32:
                lua_pushinteger(luaState, *static_cast<const int*>(data));
                return sizeof(int);
            case Engine::Core::Pipeline::VariantType::Uint32:
                lua_pushinteger(luaState, *static_cast<const unsigned int*>(data));
                return sizeof(unsigned int);
            case Engine::Core::Pipeline::VariantType::Float:
                lua_pushnumber(luaState, *static_cast<const float*>(data));
                return sizeof(float);
            case Engine::Core::Pipeline::VariantType::Vec2:
                return PushUserData<glm::vec2>(luaState, data);
            case Engine::Core::Pipeline::VariantType::Vec3:
                return PushUserData<glm::vec3>(luaState, data);
            case Engine::Core::Pipeline::VariantType::Vec4:
                return PushUserData<glm::vec4>(luaState, data);
            case Engine::Core::Pipeline::VariantType::Mat2:
                return PushUserData<glm::mat2>(luaState, data);
            case Engine::Core::Pipeline::VariantType::Mat3:
                return PushUserData<glm::mat3>(luaState, data);
            case Engine::Core::Pipeline::VariantType::Mat4:
                return PushUserData<glm::mat4>(luaState, data);
            case Engine::Core::Pipeline::VariantType::Path:
                return PushUserData<Engine::Core::Pipeline::HashId>(luaState, data);
            case Engine::Core::Pipeline::VariantType::Invalid:
                return 0;
            }
        }
    case Engine::Core::Scripting::ScriptObjectType::Struct:
        {
            lua_createtable(luaState, 0, reflection->DataType.StructType.PropertyCount);
            size_t readCounter = 0;
            
            for (size_t i = 0; i < reflection->DataType.StructType.PropertyCount; i++)
            {
                // align the address
                readCounter = Align(readCounter, reflection->DataType.StructType.Properties[i].Type->Alignment);

                lua_pushstring(luaState, reflection->DataType.StructType.Properties[i].Name);
                size_t newRead = PushScriptObject(reflection->DataType.StructType.Properties[i].Type, (static_cast<const unsigned char*>(data) + readCounter), luaState);
                lua_settable(luaState, -3);
                readCounter += newRead;
            }

            return readCounter;
        }
    case Engine::Core::Scripting::ScriptObjectType::Array:
        {
            struct ArrayHeader
            {
                const void* Data;
                size_t Length;
            };
            const ArrayHeader* header = static_cast<const ArrayHeader*>(data);

            lua_createtable(luaState, header->Length, 0);
            size_t readCounter = 0;

            for (size_t i = 0; i < header->Length; i++)
            {
                lua_pushinteger(luaState, i + 1);
                size_t newRead = PushScriptObject(reflection->DataType.ArrayType, (static_cast<const unsigned char*>(header->Data) + readCounter), luaState);
                lua_settable(luaState, -3);

                readCounter += Align(newRead, reflection->DataType.ArrayType->Alignment);
            }

            return sizeof(ArrayHeader);
        }
    default:
        return 0;
    }
}

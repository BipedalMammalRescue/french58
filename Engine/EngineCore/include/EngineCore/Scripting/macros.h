#pragma once

#include "EngineCore/Pipeline/variant.h"
#include "EngineCore/Scripting/script_object.h"
#include "glm/ext/matrix_float2x2.hpp"
#include "glm/ext/matrix_float3x3.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"

#define ADD_ARRAY_MEMBER(Member) { #Member, Engine::Core::Scripting::GetArrayMember(&CURRENT_LOCAL_OWNER_TYPE::Member) },

#define ADD_MEMBER(Member) { #Member, Engine::Core::Scripting::GetMember(&CURRENT_LOCAL_OWNER_TYPE::Member) },

#define DECLARE_SCRIPT_OBJECT(ReflectedType) template<> const Engine::Core::Scripting::ScriptObject* Engine::Core::Scripting::GetScriptObjectForType<ReflectedType>()\
{\
    using CURRENT_LOCAL_OWNER_TYPE = ReflectedType;\
    static const Engine::Core::Scripting::ScriptProperty members[] = {

#define END_SCRIPT_OBJECT };\
    static const Engine::Core::Scripting::ScriptObject object = {\
        Engine::Core::Scripting::ScriptObjectType::Struct,\
        alignof(CURRENT_LOCAL_OWNER_TYPE),\
        {\
            .StructType = {\
                members,\
                sizeof(members) / sizeof(Engine::Core::Scripting::ScriptProperty)\
            }\
        }\
    };\
    return &object;\
}

#define DECLARE_SCRIPT_CALLABLE(FuncName, ReturnType, ParamType, ParamName) \
const Engine::Core::Scripting::ScriptCallable* Get##FuncName##ScriptCallable() \
{\
    using TRetType = ReturnType; \
    using TParamType = ParamType; \
    static const char* funcName = #FuncName; \
    static const auto innerCallable = [](const Engine::Core::Runtime::ServiceTable* services, const void* moduleState, ParamType ParamName)

#define END_SCRIPT_CALLABLE ; \
    static const auto wrapperCall = [](const Engine::Core::Runtime::ServiceTable* services, const void* moduleState, Engine::Core::Scripting::IParamEnumerator* params, Engine::Core::Scripting::IReturnWriter* output) \
    { \
        TParamType param; \
        if (!params->GetNext(&param)) \
            return Engine::Core::Scripting::ScriptCallableResult::ParamMismatch; \
        TRetType result = innerCallable(services, moduleState, param); \
        output->Write(&result); \
        return Engine::Core::Scripting::ScriptCallableResult::Success; \
    }; \
    static const Engine::Core::Scripting::ScriptObject* paramTypes[] = { \
        Engine::Core::Scripting::GetScriptObjectForType<TParamType>() \
    }; \
    static const Engine::Core::Scripting::ScriptCallable callable = { \
        funcName, \
        paramTypes, \
        1, \
        Engine::Core::Scripting::GetScriptObjectForType<TRetType>(), \
        wrapperCall \
    }; \
    return &callable; \
}

namespace Engine::Core::Scripting {

template <typename T>
const Engine::Core::Scripting::ScriptObject* GetScriptObjectForType();

template<> inline const Engine::Core::Scripting::ScriptObject* GetScriptObjectForType<unsigned char>() 
{
    static const Engine::Core::Scripting::ScriptObject obj = {
        ScriptObjectType::Unary,
        alignof(unsigned char),
        {
            .UnaryType = Pipeline::ToVariantType<unsigned char>()
        }
    };

    return &obj;
}

template<> inline const Engine::Core::Scripting::ScriptObject* GetScriptObjectForType<bool>() 
{
    static const Engine::Core::Scripting::ScriptObject obj = {
        ScriptObjectType::Unary,
        alignof(bool),
        {
            .UnaryType = Pipeline::ToVariantType<bool>()
        }
    };

    return &obj;
}

template<> inline const Engine::Core::Scripting::ScriptObject* GetScriptObjectForType<int>() 
{
    static const Engine::Core::Scripting::ScriptObject obj = {
        ScriptObjectType::Unary,
        alignof(int),
        {
            .UnaryType = Pipeline::ToVariantType<int>()
        }
    };

    return &obj;
}

template<> inline const Engine::Core::Scripting::ScriptObject* GetScriptObjectForType<unsigned int>() 
{
    static const Engine::Core::Scripting::ScriptObject obj = {
        ScriptObjectType::Unary,
        alignof(unsigned int),
        {
            .UnaryType = Pipeline::ToVariantType<unsigned int>()
        }
    };

    return &obj;
}

template<> inline const Engine::Core::Scripting::ScriptObject* GetScriptObjectForType<float>() 
{
    static const Engine::Core::Scripting::ScriptObject obj = {
        ScriptObjectType::Unary,
        alignof(float),
        {
            .UnaryType = Pipeline::ToVariantType<float>()
        }
    };

    return &obj;
}

template<> inline const Engine::Core::Scripting::ScriptObject* GetScriptObjectForType<glm::vec2>() 
{
    static const Engine::Core::Scripting::ScriptObject obj = {
        ScriptObjectType::Unary,
        alignof(glm::vec2),
        {
            .UnaryType = Pipeline::ToVariantType<glm::vec2>()
        }
    };

    return &obj;
}

template<> inline const Engine::Core::Scripting::ScriptObject* GetScriptObjectForType<glm::vec3>() 
{
    static const Engine::Core::Scripting::ScriptObject obj = {
        ScriptObjectType::Unary,
        alignof(glm::vec3),
        {
            .UnaryType = Pipeline::ToVariantType<glm::vec3>()
        }
    };

    return &obj;
}

template<> inline const Engine::Core::Scripting::ScriptObject* GetScriptObjectForType<glm::vec4>() 
{
    static const Engine::Core::Scripting::ScriptObject obj = {
        ScriptObjectType::Unary,
        alignof(glm::vec4),
        {
            .UnaryType = Pipeline::ToVariantType<glm::vec4>()
        }
    };

    return &obj;
}

template<> inline const Engine::Core::Scripting::ScriptObject* GetScriptObjectForType<glm::mat2>() 
{
    static const Engine::Core::Scripting::ScriptObject obj = {
        ScriptObjectType::Unary,
        alignof(glm::mat2),
        {
            .UnaryType = Pipeline::ToVariantType<glm::mat2>()
        }
    };

    return &obj;
}

template<> inline const Engine::Core::Scripting::ScriptObject* GetScriptObjectForType<glm::mat3>() 
{
    static const Engine::Core::Scripting::ScriptObject obj = {
        ScriptObjectType::Unary,
        alignof(glm::mat3),
        {
            .UnaryType = Pipeline::ToVariantType<glm::mat3>()
        }
    };

    return &obj;
}

template<> inline const Engine::Core::Scripting::ScriptObject* GetScriptObjectForType<glm::mat4>() 
{
    static const Engine::Core::Scripting::ScriptObject obj = {
        ScriptObjectType::Unary,
        alignof(glm::mat4),
        {
            .UnaryType = Pipeline::ToVariantType<glm::mat4>()
        }
    };

    return &obj;
}

template <typename TOwner, typename TData>
const Engine::Core::Scripting::ScriptObject* GetMember(TData TOwner::*)
{
    return GetScriptObjectForType<TData>();
}

template <typename TOwner, typename TData>
const Engine::Core::Scripting::ScriptObject* GetArrayMember(ScriptArray<TData> TOwner::*)
{
    static const ScriptObject arrayType = {
        ScriptObjectType::Array,
        alignof(ScriptArray<TData>),
        {
            .ArrayType = GetScriptObjectForType<TData>()
        }
    };
    return &arrayType;
}

}
#pragma once

#include "EngineCore/Pipeline/variant.h"
namespace Engine::Core::Scripting {

enum class ScriptObjectType : unsigned char
{
    Void,
    Unary,
    Struct,
    Array
};

struct ScriptProperty;

template <typename T>
struct ScriptArray
{
    const T* Data;
    size_t Length;
};

struct ScriptObject 
{
    ScriptObjectType ObjectType;
    size_t Alignment;
    union
    {
        Pipeline::VariantType UnaryType;
        
        struct 
        {
            const ScriptProperty* Properties;
            size_t PropertyCount;
        } StructType;

        const ScriptObject* ArrayType;
    } DataType;
};

}
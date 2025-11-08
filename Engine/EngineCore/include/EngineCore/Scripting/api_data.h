#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/variant.h"
#include "glm/ext/matrix_float2x2.hpp"
#include "glm/ext/matrix_float3x3.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"

namespace Engine::Core::Runtime {
    class ServiceTable;
}


namespace Engine::Core::Scripting {

enum class ApiDataType : unsigned char
{
    Invalid,
    Variant,
    Object
};

struct ApiDataDefinition
{
    ApiDataType Type;
    // only used for variants
    Pipeline::VariantType SubType;
};

template <typename T>
ApiDataDefinition GetApiDataDefinition()
{
    return { ApiDataType::Object };
}

template<>
inline ApiDataDefinition GetApiDataDefinition<unsigned char>()
{
    return {ApiDataType::Variant, Pipeline::VariantType::Byte};
}

template<>
inline ApiDataDefinition GetApiDataDefinition<bool>()
{
    return {ApiDataType::Variant, Pipeline::VariantType::Bool};
}

template<>
inline ApiDataDefinition GetApiDataDefinition<int>()
{
    return {ApiDataType::Variant, Pipeline::VariantType::Int32};
}

template<>
inline ApiDataDefinition GetApiDataDefinition<unsigned int>()
{
    return {ApiDataType::Variant, Pipeline::VariantType::Uint32};
}

template<>
inline ApiDataDefinition GetApiDataDefinition<float>()
{
    return {ApiDataType::Variant, Pipeline::VariantType::Float};
}

template<>
inline ApiDataDefinition GetApiDataDefinition<glm::vec2>()
{
    return {ApiDataType::Variant, Pipeline::VariantType::Vec2};
}

template<>
inline ApiDataDefinition GetApiDataDefinition<glm::vec3>()
{
    return {ApiDataType::Variant, Pipeline::VariantType::Vec3};
}

template<>
inline ApiDataDefinition GetApiDataDefinition<glm::vec4>()
{
    return {ApiDataType::Variant, Pipeline::VariantType::Vec4};
}

template<>
inline ApiDataDefinition GetApiDataDefinition<glm::mat2>()
{
    return {ApiDataType::Variant, Pipeline::VariantType::Mat2};
}

template<>
inline ApiDataDefinition GetApiDataDefinition<glm::mat3>()
{
    return {ApiDataType::Variant, Pipeline::VariantType::Mat3};
}

template<>
inline ApiDataDefinition GetApiDataDefinition<glm::mat4>()
{
    return {ApiDataType::Variant, Pipeline::VariantType::Mat4};
}

template<>
inline ApiDataDefinition GetApiDataDefinition<Pipeline::HashId>()
{
    return {ApiDataType::Variant, Pipeline::VariantType::Path};
}

struct ApiData
{
    ApiDataType Type;
    union {
        Pipeline::Variant Variant;
        const void* Object;
    } Data;
};

template <typename T>
const T* FromApiData(const ApiData* source)
{
    // the default case is an object
    return static_cast<const T*>(source->Data.Object);
}

template<>
inline const unsigned char* FromApiData<unsigned char>(const ApiData* source)
{
    return &source->Data.Variant.Data.Byte;
}

template<>
inline const bool* FromApiData<bool>(const ApiData* source)
{
    return &source->Data.Variant.Data.Bool;
}

template<>
inline const int* FromApiData<int>(const ApiData* source)
{
    return &source->Data.Variant.Data.Int32;
}

template<>
inline const unsigned int* FromApiData<unsigned int>(const ApiData* source)
{
    return &source->Data.Variant.Data.Uint32;
}

template<>
inline const float* FromApiData<float>(const ApiData* source)
{
    return &source->Data.Variant.Data.Float;
}

template<>
inline const glm::vec2* FromApiData<glm::vec2>(const ApiData* source)
{
    return &source->Data.Variant.Data.Vec2;
}

template<>
inline const glm::vec3* FromApiData<glm::vec3>(const ApiData* source)
{
    return &source->Data.Variant.Data.Vec3;
}

template<>
inline const glm::vec4* FromApiData<glm::vec4>(const ApiData* source)
{
    return &source->Data.Variant.Data.Vec4;
}

template<>
inline const glm::mat2* FromApiData<glm::mat2>(const ApiData* source)
{
    return &source->Data.Variant.Data.Mat2;
}

template<>
inline const glm::mat3* FromApiData<glm::mat3>(const ApiData* source)
{
    return &source->Data.Variant.Data.Mat3;
}

template<>
inline const glm::mat4* FromApiData<glm::mat4>(const ApiData* source)
{
    return &source->Data.Variant.Data.Mat4;
}

template<>
inline const Pipeline::HashId* FromApiData<Pipeline::HashId>(const ApiData* source)
{
    return &source->Data.Variant.Data.Path;
}


template <typename T>
ApiData ToApiData(const T* source)
{
    // the default case is an object
    return ApiData { .Type = ApiDataType::Object, .Data = { .Object = source } };
}

template <>
inline ApiData ToApiData<unsigned char>(const unsigned char* source)
{
    ApiData result { ApiDataType::Variant };
    result.Data.Variant.Type = Pipeline::VariantType::Byte;
    result.Data.Variant.Data.Byte = *source;
    return result;
}

template <>
inline ApiData ToApiData<bool>(const bool* source)
{
    ApiData result { ApiDataType::Variant };
    result.Data.Variant.Type = Pipeline::VariantType::Bool;
    result.Data.Variant.Data.Bool = *source;
    return result;
}

template <>
inline ApiData ToApiData<int>(const int* source)
{
    ApiData result { ApiDataType::Variant };
    result.Data.Variant.Type = Pipeline::VariantType::Int32;
    result.Data.Variant.Data.Int32 = *source;
    return result;
}

template <>
inline ApiData ToApiData<float>(const float* source)
{
    ApiData result { ApiDataType::Variant };
    result.Data.Variant.Type = Pipeline::VariantType::Float;
    result.Data.Variant.Data.Float = *source;
    return result;
}

template <>
inline ApiData ToApiData<glm::vec2>(const glm::vec2* source)
{
    ApiData result { ApiDataType::Variant };
    result.Data.Variant.Type = Pipeline::VariantType::Vec2;
    result.Data.Variant.Data.Vec2 = *source;
    return result;
}

template <>
inline ApiData ToApiData<glm::vec3>(const glm::vec3* source)
{
    ApiData result { ApiDataType::Variant };
    result.Data.Variant.Type = Pipeline::VariantType::Vec3;
    result.Data.Variant.Data.Vec3 = *source;
    return result;
}

template <>
inline ApiData ToApiData<glm::vec4>(const glm::vec4* source)
{
    ApiData result { ApiDataType::Variant };
    result.Data.Variant.Type = Pipeline::VariantType::Vec4;
    result.Data.Variant.Data.Vec4 = *source;
    return result;
}

template <>
inline ApiData ToApiData<glm::mat2>(const glm::mat2* source)
{
    ApiData result { ApiDataType::Variant };
    result.Data.Variant.Type = Pipeline::VariantType::Mat2;
    result.Data.Variant.Data.Mat2 = *source;
    return result;
}

template <>
inline ApiData ToApiData<glm::mat3>(const glm::mat3* source)
{
    ApiData result { ApiDataType::Variant };
    result.Data.Variant.Type = Pipeline::VariantType::Mat3;
    result.Data.Variant.Data.Mat3 = *source;
    return result;
}

template <>
inline ApiData ToApiData<glm::mat4>(const glm::mat4* source)
{
    ApiData result { ApiDataType::Variant };
    result.Data.Variant.Type = Pipeline::VariantType::Mat4;
    result.Data.Variant.Data.Mat4 = *source;
    return result;
}

template <>
inline ApiData ToApiData<Pipeline::HashId>(const Pipeline::HashId* source)
{
    ApiData result { ApiDataType::Variant };
    result.Data.Variant.Type = Pipeline::VariantType::Path;
    result.Data.Variant.Data.Path = *source;
    return result;
}


template <typename T>
bool CheckOpaqueObjectType(const Runtime::ServiceTable*, const void*, const void*);

// dynamic type checking
template <typename T>
bool IsTypeCompiliant(const Runtime::ServiceTable* services, const void* moduleState, const ApiData* source)
{
    // default case is for generic object
    return source->Type == ApiDataType::Object && CheckOpaqueObjectType<T>(services, moduleState, source->Data.Object);
}

template <>
inline bool IsTypeCompiliant<unsigned char>(const Runtime::ServiceTable*, const void*, const ApiData* source)
{
    return source->Type == ApiDataType::Variant && source->Data.Variant.Type == Pipeline::VariantType::Byte;
}

template <>
inline bool IsTypeCompiliant<bool>(const Runtime::ServiceTable*, const void*, const ApiData* source)
{
    return source->Type == ApiDataType::Variant && source->Data.Variant.Type == Pipeline::VariantType::Bool;
}

template <>
inline bool IsTypeCompiliant<int>(const Runtime::ServiceTable*, const void*, const ApiData* source)
{
    return source->Type == ApiDataType::Variant && source->Data.Variant.Type == Pipeline::VariantType::Int32;
}

template <>
inline bool IsTypeCompiliant<unsigned int>(const Runtime::ServiceTable*, const void*, const ApiData* source)
{
    return source->Type == ApiDataType::Variant && source->Data.Variant.Type == Pipeline::VariantType::Uint32;
}

template <>
inline bool IsTypeCompiliant<float>(const Runtime::ServiceTable*, const void*, const ApiData* source)
{
    return source->Type == ApiDataType::Variant && source->Data.Variant.Type == Pipeline::VariantType::Float;
}

template <>
inline bool IsTypeCompiliant<glm::vec2>(const Runtime::ServiceTable*, const void*, const ApiData* source)
{
    return source->Type == ApiDataType::Variant && source->Data.Variant.Type == Pipeline::VariantType::Vec2;
}

template <>
inline bool IsTypeCompiliant<glm::vec3>(const Runtime::ServiceTable*, const void*, const ApiData* source)
{
    return source->Type == ApiDataType::Variant && source->Data.Variant.Type == Pipeline::VariantType::Vec3;
}

template <>
inline bool IsTypeCompiliant<glm::vec4>(const Runtime::ServiceTable*, const void*, const ApiData* source)
{
    return source->Type == ApiDataType::Variant && source->Data.Variant.Type == Pipeline::VariantType::Vec4;
}

template <>
inline bool IsTypeCompiliant<glm::mat2>(const Runtime::ServiceTable*, const void*, const ApiData* source)
{
    return source->Type == ApiDataType::Variant && source->Data.Variant.Type == Pipeline::VariantType::Mat2;
}

template <>
inline bool IsTypeCompiliant<glm::mat3>(const Runtime::ServiceTable*, const void*, const ApiData* source)
{
    return source->Type == ApiDataType::Variant && source->Data.Variant.Type == Pipeline::VariantType::Mat3;
}

template <>
inline bool IsTypeCompiliant<glm::mat4>(const Runtime::ServiceTable*, const void*, const ApiData* source)
{
    return source->Type == ApiDataType::Variant && source->Data.Variant.Type == Pipeline::VariantType::Mat4;
}

template <>
inline bool IsTypeCompiliant<Pipeline::HashId>(const Runtime::ServiceTable*, const void*, const ApiData* source)
{
    return source->Type == ApiDataType::Variant && source->Data.Variant.Type == Pipeline::VariantType::Path;
}

#define RC(type) template<> \
struct ReturnContainer<type> \
{ \
    const type Data; \
    const type* Get() const { return &Data; } \
};

template <typename T>
struct ReturnContainer
{
    const T* Data;
    const T* Get() const { return Data; }
};

template <> struct ReturnContainer<unsigned char> 
{
  const unsigned char Data;
  const unsigned char *Get() const { return &Data; }
};

template <> struct ReturnContainer<bool> 
{
  const bool Data;
  const bool *Get() const { return &Data; }
};

template <> 
struct ReturnContainer<int> 
{
    const int Data;
    const int *Get() const { return &Data; }
};

template <> 
struct ReturnContainer<unsigned int> 
{
    const unsigned int Data;
    const unsigned int *Get() const { return &Data; }
};

template <> 
struct ReturnContainer<float> 
{
    const float Data;
    const float *Get() const { return &Data; }
};

template <> struct ReturnContainer<glm ::vec2> 
{
    const glm ::vec2 Data;
    const glm ::vec2 *Get() const { return &Data; }
};

template <> 
struct ReturnContainer<glm ::vec3> 
{
    const glm ::vec3 Data;
    const glm ::vec3 *Get() const { return &Data; }
};

template <> 
struct ReturnContainer<glm ::vec4> 
{
    const glm ::vec4 Data;
    const glm ::vec4 *Get() const { return &Data; }
};

template <> 
struct ReturnContainer<glm ::mat2> 
{
    const glm ::mat2 Data;
    const glm ::mat2 *Get() const { return &Data; }
};

template <> 
struct ReturnContainer<glm ::mat3> 
{
    const glm ::mat3 Data;
    const glm ::mat3 *Get() const { return &Data; }
};

template <> 
struct ReturnContainer<glm ::mat4> 
{
    const glm ::mat4 Data;
    const glm ::mat4 *Get() const { return &Data; }
};

template <> 
struct ReturnContainer<Pipeline ::HashId> 
{
    const Pipeline ::HashId Data;
    const Pipeline ::HashId *Get() const { return &Data; }
};

}
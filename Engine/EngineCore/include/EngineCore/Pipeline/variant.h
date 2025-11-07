#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include "glm/ext/matrix_float2x2.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat2x2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <cstdint>
#include <array>

namespace Engine::Core::Pipeline {

enum class VariantType : unsigned char
{
    Byte,
    Bool,
    Int32,
    // Int64,
    Uint32,
    // Uint64,
    Float,
    Vec2,
    Vec3,
    Vec4,
    Mat2,
    Mat3,
    Mat4,
    Path,
    Invalid
};

struct Variant 
{
    VariantType Type;
    union {
        unsigned char Byte;
        bool Bool;
        int32_t Int32;
        int64_t Int64;
        uint32_t Uint32;
        uint64_t Uint64;
        float Float;
        glm::vec2 Vec2;
        glm::vec3 Vec3;
        glm::vec4 Vec4;
        glm::mat2 Mat2;
        glm::mat3 Mat3;
        glm::mat4 Mat4;
        std::array<unsigned char, 16> Path;
    } Data;

    Variant() = default;
    Variant(const unsigned char& byte) : Type(VariantType::Byte), Data({.Byte=byte}) {}
    Variant(const bool& boolean) : Type(VariantType::Bool), Data({.Bool=boolean}) {}
    Variant(const int32_t& int32) : Type(VariantType::Int32), Data({.Int32=int32}) {}
    // Variant(const int64_t& int64) : Type(VariantType::Int64), Data({.Int64=int64}) {}
    Variant(const uint32_t& uint32) : Type(VariantType::Uint32), Data({.Uint32=uint32}) {}
    // Variant(const uint64_t& uint64) : Type(VariantType::Uint64), Data({.Uint64=uint64}) {}
    Variant(const float& single) : Type(VariantType::Float), Data({.Float=single}) {}
    Variant(const glm::vec2& vec2) : Type(VariantType::Vec2), Data({.Vec2=vec2}) {}
    Variant(const glm::vec3& vec3) : Type(VariantType::Vec3), Data({.Vec3=vec3}) {}
    Variant(const glm::vec4& vec4) : Type(VariantType::Vec4), Data({.Vec4=vec4}) {}
    Variant(const glm::mat2& mat2) : Type(VariantType::Mat2), Data({.Mat2=mat2}) {}
    Variant(const glm::mat3& mat3) : Type(VariantType::Mat3), Data({.Mat3=mat3}) {}
    Variant(const glm::mat4& mat4) : Type(VariantType::Mat4), Data({.Mat4=mat4}) {}
    Variant(const HashId& path) : Type(VariantType::Path), Data({.Path=path.Hash}) {}
};

size_t GetVariantPayloadSize(const Variant& source);

template <typename T>
VariantType ToVariantType();

template<> inline VariantType ToVariantType<unsigned char>() { return VariantType::Byte; }
template<> inline VariantType ToVariantType<bool>() { return VariantType::Bool; }
template<> inline VariantType ToVariantType<int>() { return VariantType::Int32; }
template<> inline VariantType ToVariantType<unsigned int>() { return VariantType::Uint32; }
template<> inline VariantType ToVariantType<float>() { return VariantType::Float; }
template<> inline VariantType ToVariantType<glm::vec2>() { return VariantType::Vec2; }
template<> inline VariantType ToVariantType<glm::vec3>() { return VariantType::Vec3; }
template<> inline VariantType ToVariantType<glm::vec4>() { return VariantType::Vec4; }
template<> inline VariantType ToVariantType<glm::mat2>() { return VariantType::Mat2; }
template<> inline VariantType ToVariantType<glm::mat3>() { return VariantType::Mat3; }
template<> inline VariantType ToVariantType<glm::mat4>() { return VariantType::Mat4; }

}
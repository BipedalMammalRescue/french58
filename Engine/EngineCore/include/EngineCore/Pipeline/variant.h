#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat2x2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <cstdint>
#include <array>

namespace Engine::Core::Pipeline {

enum class VariantType 
{
    Byte,
    Bool,
    Int32,
    Int64,
    Uint32,
    Uint64,
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
        int32_t Int32;
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
};

}
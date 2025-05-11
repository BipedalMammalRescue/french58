#pragma once

#include <glm/fwd.hpp>
#include <glm/glm.hpp>

namespace Engine {
namespace Core {
namespace Pipeline {
namespace Scripting {

// one of the ugliest ways to achieve runtime polymorphism
// maybe not I've literally been using type codes at work too
enum class DataType
{
    BYTE,
    INT32,
    INT64,
    UINT32,
    UINT64,
    FLOAT,
    VEC2,
    VEC3,
    VEC4,
    MAT2,
    MAT3,
    MAT4
};

// inspired by godot's scripting layer but mine only slows down the editor
struct Variant
{
    DataType Type;
    union {
        unsigned char Byte;
        int32_t Int32;
        int64_t Int64;
        uint32_t UInt32;
        uint64_t UInt64;
        float Float;
        double Double;
        glm::vec2 Vec2;
        glm::vec3 Vec3;
        glm::vec4 Vec4;
        glm::mat2 Mat2;
        glm::mat3 Mat3;
        glm::mat4 Mat4;
    } Data;
};

} // namespace Scripting
} // namespace Pipeline
} // namespace Core
} // namespace Engine

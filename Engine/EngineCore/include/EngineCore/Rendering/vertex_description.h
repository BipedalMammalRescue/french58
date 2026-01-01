#pragma once

#include <cstdint>
namespace Engine::Core::Rendering {

enum class VertexDataTypes
{
    Float,
    Vector2,
    Vector3,
    Vector4
};

struct VertexBinding
{
    VertexDataTypes *Attributes;
    uint32_t AttributeCount;
};

struct VertexDescription
{
    VertexBinding *Bindings;
    uint32_t BindingCount;
};

enum class IndexType
{
    Ubyte,
    Ushort,
    Uint
};

} // namespace Engine::Core::Rendering
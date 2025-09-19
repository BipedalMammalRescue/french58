#pragma once

#include <SDL3/SDL_gpu.h>
#include <unordered_map>
#include <string>

namespace Engine::Core::Runtime {

class RendererService;

}

namespace Engine::Core::Rendering {

enum class ShaderType
{
    FRAGMENT_SHADER,
    VERTEX_SHADER
};

enum class GpuDataType
{
    INT32,
    UINT32,
    FLOAT,
};

struct VertexAttribute
{
    GpuDataType Type;
    unsigned int Count;
};

struct VertexCollection
{
    const void *Vertices;
    unsigned int Size;
};

// TODO: somehow use layouts to index into a pipeline cache?
struct VertexLayout
{
    VertexAttribute *Elements;
    unsigned int Count;

    unsigned int GetSize() const
    {
        return sizeof(VertexAttribute) * Count;
    }
};

struct IndexCollection
{
    const unsigned int *Indices;
    unsigned int Count;

    unsigned int GetSize() const
    {
        return sizeof(unsigned int) * Count;
    }
};

enum class ShaderParamType
{
    INT32,
    UINT32,
    FLOAT,
    DOUBLE,
    VEC2,
    VEC3,
    VEC4,
    MAT2,
    MAT3,
    MAT4
};

class RendererShader
{
  private:
    friend class Engine::Core::Runtime::RendererService;
    SDL_GPUShader *m_ShaderID = nullptr;
};

class RendererMaterial
{
  private:
    friend class Engine::Core::Runtime::RendererService;
    SDL_GPUGraphicsPipeline *m_ProgramID = 0;
    std::unordered_map<std::string, int> m_UniformLocationCache;
};

class RendererMesh
{
  private:
    friend class Engine::Core::Runtime::RendererService;
    SDL_GPUBuffer *m_VertexBuffer;
    SDL_GPUBuffer *m_IndexBuffer;
    unsigned int m_IndexCount;
};

} // namespace Engine::Core::Rendering

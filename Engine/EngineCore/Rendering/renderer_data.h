#pragma once

#include <SDL3/SDL_gpu.h>

namespace Engine {
namespace Core {
namespace Rendering {

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
	DOUBLE,
	BYTE
};

struct VertexBufferLayoutElement
{
	GpuDataType Type;
	unsigned int Count;
	bool Normalized;
};

struct VertexCollection
{
	const void* Vertices;
	unsigned int Size;
};

// TODO: somehow use layouts to index into a pipeline cache?
struct VertexLayout
{
	VertexBufferLayoutElement* Elements;
	unsigned int Count;

	unsigned int GetSize() const
	{
		return sizeof(VertexBufferLayoutElement) * Count;
	}
};

struct IndexCollection
{
	const unsigned int* Indices;
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

/// <summary>
/// Denote parameters passed in during QueueRender
/// </summary>
struct DynamicShaderParameter
{
	const std::string& Name;
	void* Data;
	ShaderParamType Type;
};

}
}
}

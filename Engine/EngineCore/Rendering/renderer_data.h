#pragma once

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
};

struct VertexAttribute
{
	GpuDataType Type;
	unsigned int Count;
};

struct VertexCollection
{
	const void* Vertices;
	unsigned int Size;
};

// TODO: somehow use layouts to index into a pipeline cache?
struct VertexLayout
{
	VertexAttribute* Elements;
	unsigned int Count;

	unsigned int GetSize() const
	{
		return sizeof(VertexAttribute) * Count;
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

}
}
}

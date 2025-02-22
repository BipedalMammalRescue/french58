#include "renderer_service.h"
#include "ErrorHandling/exceptions.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <stdio.h>
#include <string>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat2x2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <SDL3/SDL_gpu.h>

#define GLCallLite(routine) ClearGLErrors();\
	routine;\
	return CheckGLError(#routine, __FILE__, __LINE__)

#define GLCall(routine) ClearGLErrors();\
	routine;\
	if (!CheckGLError(#routine, __FILE__, __LINE__))\
		goto OnError


using namespace Engine;
using namespace Engine::Core;


static const char s_ChannelName[] = "RendererService";


static void ClearGLErrors()
{
	while (glGetError() != GL_NO_ERROR);
}

static unsigned int TranslateShaderType(Rendering::ShaderType type)
{
	switch (type)
	{
	case Rendering::ShaderType::FRAGMENT_SHADER:
		return GL_FRAGMENT_SHADER;
	case Rendering::ShaderType::VERTEX_SHADER:
		return GL_VERTEX_SHADER;
	default:
		SE_THROW_NOT_IMPLEMENTED;
	}
}


static const char* PrintShaderType(Rendering::ShaderType type)
{
	static const char s_ShaderTypeFrag[] = "FRAGMENT";
	static const char s_ShaderTypeVert[] = "VERTEX";

	switch (type)
	{
	case Rendering::ShaderType::FRAGMENT_SHADER:
		return s_ShaderTypeFrag;
	case Rendering::ShaderType::VERTEX_SHADER:
		return s_ShaderTypeVert;
	default:
		SE_THROW_NOT_IMPLEMENTED;
	}
}


static unsigned int GetGpuDataSize(Rendering::GpuDataType type)
{
	switch (type)
	{
	case Rendering::GpuDataType::INT32:
		return sizeof(GLint);
	case Rendering::GpuDataType::UINT32:
		return sizeof(GLuint);
	case Rendering::GpuDataType::FLOAT:
		return sizeof(GLfloat);
	case Rendering::GpuDataType::DOUBLE:
		return sizeof(GLdouble);
	case Rendering::GpuDataType::BYTE:
		return sizeof(GLbyte);
	default:
		return 0;
	}
}


static unsigned int GetStride(const Rendering::VertexLayout& layout)
{
	unsigned int stride = 0;
	for (unsigned int i = 0; i < layout.Length; i++)
	{
		stride += layout.Elements[i].Count * GetGpuDataSize(layout.Elements[i].Type);
	}
	return stride;
}


bool Rendering::RendererService::CheckGLError(const char* function, const char* file, int line)
{
	return false;
}


bool Engine::Core::Rendering::RendererService::DeleteShader(RendererShader& shader)
{
	return false;
}


bool Engine::Core::Rendering::RendererService::CreateMaterial(const RendererShader& vertexShader, const RendererShader& fragmentShader, RendererMaterial& outID)
{
	return false;
}


bool Engine::Core::Rendering::RendererService::DeleteMaterial(RendererMaterial& material)
{
	return false;
}


bool Engine::Core::Rendering::RendererService::RegisterMesh(const VertexCollection& vertices, const VertexLayout& vertexLayout, const IndexCollection& indices, RendererMesh& outID)
{
	return false;
}


bool Engine::Core::Rendering::RendererService::DeleteMesh(RendererMesh& inID)
{
	return false;
}


bool Rendering::RendererService::ApplyDynamicShaderParameter(const Rendering::DynamicShaderParameter* shaderParam, Rendering::RendererMaterial* material)
{
	return false;
}


bool Rendering::RendererService::QueueRender(RendererMesh* mesh, RendererMaterial* material, const Rendering::DynamicShaderParameter* shaderParams, unsigned int shaderParamCount)
{
	return false;
}


bool Engine::Core::Rendering::RendererMesh::Bail()
{
	return false;
}

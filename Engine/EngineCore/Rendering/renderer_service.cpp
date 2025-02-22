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


using namespace Engine;
using namespace Engine::Core;


static const char s_ChannelName[] = "RendererService";

bool Rendering::RendererService::CheckGLError(const char* function, const char* file, int line)
{
	return false;
}

bool Engine::Core::Rendering::RendererService::CompileShader(const std::string &code, ShaderType type, RendererShader &outID)
{
    return false;
}

bool Engine::Core::Rendering::RendererService::DeleteShader(RendererShader &shader)
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

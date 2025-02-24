#pragma once

#include "Logging/logger_service.h"
#include "Platform/platform_access.h"
#include "SDL3/SDL_gpu.h"
#include "glm/fwd.hpp"
#include "renderer_data.h"

#include <string>
#include <unordered_map>


struct SDL_GPUBuffer;
struct SDL_GPUShader;

namespace Engine {
namespace Core {
namespace Rendering {


class RendererShader
{
private:
	friend class RendererService;
	SDL_GPUShader* m_ShaderID = nullptr;
};


class RendererMaterial
{
private:
	friend class RendererService;
	SDL_GPUGraphicsPipeline* m_ProgramID = 0;
	std::unordered_map<std::string, int> m_UniformLocationCache;
};


/// <summary>
/// Encapsulates an internal representation of data structures and contracts between the renderer service and graphics backend.
/// Currently it's just VB, VA and IB tokens from OpenGL.
/// </summary>
class RendererMesh
{
private:
	friend class RendererService;
	SDL_GPUBuffer* m_VertexBuffer;
	SDL_GPUBuffer* m_IndexBuffer;
	unsigned int m_IndexCount;
};


// Renderer service owns another set of SDL api outside of PlatformAccess as the ultimate endpoint for any rendering behavior.
// TODO: add render texture
class RendererService
{
private:
	Logging::LoggerService* m_Logger = Logging::GetLogger();
	Platform::PlatformAccess* m_Platform = nullptr;

public:
	RendererService(Platform::PlatformAccess* platform)
		: m_Platform(platform)
	{}
	
	~RendererService() = default;

public:
	bool CompileShader(const unsigned char* code, size_t codeLength, ShaderType type, unsigned int numSamplers, unsigned int numUniformBuffers, unsigned int numStorageBuffers, unsigned int numSotrageTextures, RendererShader &outID);
	bool DeleteShader(RendererShader& shader);

	bool CreateMaterial(const RendererShader& vertexShader, const RendererShader& fragmentShader, VertexAttribute* layouts, unsigned int layoutCount, RendererMaterial& outID);
	bool DeleteMaterial(RendererMaterial& material);

	bool RegisterMesh(const VertexCollection& vertices, const IndexCollection& indices, RendererMesh& outID);
	bool DeleteMesh(RendererMesh& inID);

	// rendering regular object would only require a MVP, and every object we assume has an MVP
	// maybe signature-annotate the puroose of this function?
	// TODO: how should this api work with uniforms?
	// WE CAN technically get some temp solutions?
	bool QueueRender(RendererMesh* mesh, RendererMaterial* material, const glm::mat4& mvp);
};

}
}
}

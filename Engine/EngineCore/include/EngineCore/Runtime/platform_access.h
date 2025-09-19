#pragma once

#include "EngineCore/Configuration/configuration_provider.h"
#include "EngineCore/Logging/logger_service.h"

#include <glm/fwd.hpp>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_gpu.h>

struct SDL_Window;

namespace Engine::Core::Rendering {

class RendererMesh;
class RendererMaterial;

}

namespace Engine::Core::Runtime {

class RendererService;

/// <summary>
/// PlatformAccess handles *almost* everything platform-specific.
/// This is the first service that other services depend on, it's not a client-facing service.
/// Currently: this is just a SDL wrapper.
/// </summary>
class PlatformAccess
{
	// injected
private:
	const Configuration::ConfigurationProvider* m_Configs = nullptr;

	// initialized
private:
	friend class Engine::Core::Runtime::RendererService;
	SDL_Window* m_Window = nullptr;
	SDL_GPUDevice* m_GpuDevice = nullptr;
    SDL_GPUCommandBuffer* m_CommandBuffer = nullptr;
    SDL_GPUTexture* m_SwapchainTexture = nullptr;
    Logging::LoggerService* m_Logger = Logging::GetLogger();

public:
    bool InitializeSDL();
	void BeginFrame();
    void CreateRenderPass(Rendering::RendererMesh *mesh, Rendering::RendererMaterial *material, const glm::mat4 &mvp);
	void EndFrame();
	PlatformAccess(const Configuration::ConfigurationProvider* configs);
	~PlatformAccess();
};

}
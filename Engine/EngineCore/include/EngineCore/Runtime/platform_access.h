#pragma once

#include "EngineCore/Configuration/configuration_provider.h"

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_gpu.h>

struct SDL_Window;

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
	const DependencyInjection::ConfigurationProvider* m_Configs = nullptr;

	// initialized
private:
	friend class Engine::Core::Runtime::RendererService;
	SDL_Window* m_Window = nullptr;
	SDL_GPUDevice* m_GpuDevice = nullptr;
	unsigned int m_ShaderProgram;
	int gVertexPos2DLocation;
	unsigned int m_VBO;
	unsigned int m_IBO;

private:
	bool InitializeSDL();

public:
	void BeginFrame();
	void EndFrame();
	PlatformAccess(const DependencyInjection::ConfigurationProvider* configs);
	~PlatformAccess();
};

}
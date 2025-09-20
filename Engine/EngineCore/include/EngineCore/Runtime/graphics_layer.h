#pragma once

#include "EngineCore/Configuration/configuration_provider.h"
#include "EngineCore/Logging/logger_service.h"

#include <glm/fwd.hpp>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_gpu.h>

struct SDL_Window;

namespace Engine::Core::Runtime {

class GameLoop;

// Contains states and accesses to graphics related concepts, managed by the game loop.
class GraphicsLayer
{
	// injected
private:
	const Configuration::ConfigurationProvider* m_Configs = nullptr;

	// initialized
private:
	SDL_Window* m_Window = nullptr;
	SDL_GPUDevice* m_GpuDevice = nullptr;
    SDL_GPUCommandBuffer* m_CommandBuffer = nullptr;
    SDL_GPUTexture* m_SwapchainTexture = nullptr;
    Logging::LoggerService* m_Logger = Logging::GetLogger();

    // for game loop to directly control graphics behavior
private:
    friend class GameLoop;
    bool InitializeSDL();
	void BeginFrame();
	void EndFrame();
    GraphicsLayer(const Configuration::ConfigurationProvider* configs);

public:
	~GraphicsLayer();

    inline SDL_Window* GetWindow() { return m_Window; }
    inline SDL_GPUDevice* GetDevice() { return m_GpuDevice; }

    SDL_GPURenderPass* AddRenderPass();
    void CommitRenderPass(SDL_GPURenderPass* pass);
};

}
#pragma once

#include "EngineCore/Configuration/configuration_provider.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Runtime/crash_dump.h"

#include <glm/fwd.hpp>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_gpu.h>

struct SDL_Window;

namespace Engine::Core::Logging {
class LoggerService;
}

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
    SDL_GPUTexture* m_DepthBuffer = nullptr;
    Logging::Logger m_Logger;

    // for game loop to directly control graphics behavior
private:
    friend class GameLoop;
    CallbackResult InitializeSDL();
	CallbackResult BeginFrame();
	CallbackResult EndFrame();
    GraphicsLayer(const Configuration::ConfigurationProvider* configs, Logging::LoggerService* loggerService);

public:
	~GraphicsLayer();

    inline SDL_Window* GetWindow() const { return m_Window; }
    inline SDL_GPUDevice* GetDevice() const { return m_GpuDevice; }

    inline SDL_GPUTexture* GetSharedDepthBuffer() const { return m_DepthBuffer; }
    inline SDL_GPUCommandBuffer* GetCurrentCommandBuffer() { return m_CommandBuffer; }
    SDL_GPURenderPass* AddRenderPass();
    void CommitRenderPass(SDL_GPURenderPass* pass);
};

}
#include "EngineCore/Runtime/graphics_layer.h"
#include "EngineCore/Logging/logger_service.h"
#include "EngineUtils/ErrorHandling/exceptions.h"
#include "SDL3/SDL_gpu.h"

#include <SDL3/SDL.h>

using namespace Engine::Core;
using namespace Engine::Core::Runtime;

static const char s_ServiceName[] = "PlatformAccess";
static const char s_SDLInitializationError[] = "SDL initialization failed, see logs for details.";
static const char s_GLInitializationError[] = "OpenGL initialization failed, see logs for details.";

bool Engine::Core::Runtime::GraphicsLayer::InitializeSDL()
{
	// initialize sdl
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		Logging::GetLogger()->Error(s_ServiceName, "SDL could not initialize!SDL Error: %s", SDL_GetError());
		return false;
	}

	// Create window
	m_Window = SDL_CreateWindow("Foobar Game",  m_Configs->WindowWidth, m_Configs->WindowHeight, 0);
	if (m_Window == nullptr)
	{
		Logging::GetLogger()->Error(s_ServiceName, "Window could not be created! SDL Error: %s", SDL_GetError());
		return false;
	}

	// Create GPU device
	m_GpuDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, m_Configs->UseDeviceValidation, NULL);
	if (m_GpuDevice == nullptr) 
	{
		Logging::GetLogger()->Error(s_ServiceName, "Failed to create GPU device! SDL Error: %s", SDL_GetError());
		return false;
	}
	
	// setup gpu window
	if (!SDL_ClaimWindowForGPUDevice(m_GpuDevice, m_Window))
	{
		Logging::GetLogger()->Error(s_ServiceName, "Failed to initialize GPU accelerated window! SDL Error: %s", SDL_GetError());
		return false;
	}
	
	SDL_SetWindowResizable(m_Window, false);
    SDL_ShowWindow(m_Window);
	return true;
}

void Engine::Core::Runtime::GraphicsLayer::BeginFrame()
{
    // create command buffer
    m_CommandBuffer = SDL_AcquireGPUCommandBuffer(m_GpuDevice);
    if (m_CommandBuffer == NULL)
    {
        m_Logger->Error("PlatformAccess", "AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        SE_THROW_GRAPHICS_EXCEPTION;
    }

    // get a target texture
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(m_CommandBuffer, m_Window, &m_SwapchainTexture, nullptr, nullptr))
    {
        m_Logger->Error("PlatformAccess", "WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        SE_THROW_GRAPHICS_EXCEPTION;
    }

    // create a single pass to clear screen
    SDL_GPUColorTargetInfo colorTargetInfo = {0};
    colorTargetInfo.texture = m_SwapchainTexture;
    colorTargetInfo.clear_color = SDL_FColor{1.0f, 0.0f, 1.0f, 1.0f};
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(m_CommandBuffer, &colorTargetInfo, 1, NULL);
    SDL_EndGPURenderPass(pass);
}

void Engine::Core::Runtime::GraphicsLayer::EndFrame()
{
	SDL_SubmitGPUCommandBuffer(m_CommandBuffer);
    m_CommandBuffer = nullptr;
    m_SwapchainTexture = nullptr;
}

GraphicsLayer::GraphicsLayer(const Configuration::ConfigurationProvider* configs)
	: m_Configs(configs) {}

Engine::Core::Runtime::GraphicsLayer::~GraphicsLayer()
{
	// release gpu device
	SDL_ReleaseWindowFromGPUDevice(m_GpuDevice, m_Window);
	SDL_DestroyGPUDevice(m_GpuDevice);

	//Destroy window	
	SDL_DestroyWindow(m_Window);
	m_Window = NULL;

	//Quit SDL subsystems
	SDL_Quit();
}

SDL_GPURenderPass* GraphicsLayer::AddRenderPass() 
{
    if (m_SwapchainTexture == nullptr || m_CommandBuffer == nullptr)
        SE_THROW_GRAPHICS_EXCEPTION;

    // create render pass
    SDL_GPUColorTargetInfo colorTargetInfo {0};
    colorTargetInfo.texture = m_SwapchainTexture;
    colorTargetInfo.load_op = SDL_GPU_LOADOP_LOAD;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass *renderPass = SDL_BeginGPURenderPass(m_CommandBuffer, &colorTargetInfo, 1, NULL);
    return renderPass;
}

void GraphicsLayer::CommitRenderPass(SDL_GPURenderPass* pass) 
{
    SDL_EndGPURenderPass(pass);
}
#include "EngineCore/Runtime/graphics_layer.h"
#include "EngineCore/Logging/logger_service.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineUtils/ErrorHandling/exceptions.h"
#include "SDL3/SDL_error.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL.h>
#include <cstdint>
#include <cstring>

using namespace Engine::Core;
using namespace Engine::Core::Runtime;

static const char* LogChannels[] = { "GraphicsLayer" };

#define SdlCrashOut(header) Crash(__FILE__, __LINE__, WrapSdlError(header))

std::string WrapSdlError(const char* header)
{
    std::string lastwords(header);
    lastwords.append(" SDL error: ");
    lastwords.append(SDL_GetError());
    return lastwords;
}

CallbackResult Engine::Core::Runtime::GraphicsLayer::InitializeSDL()
{
	// Create window
	m_Window = SDL_CreateWindow("Foobar Game",  m_Configs->WindowWidth, m_Configs->WindowHeight, 0);
	if (m_Window == nullptr)
	{
        static const char errorMessage[] = "Window creation failed.";
        m_Logger.Fatal(errorMessage);
		return SdlCrashOut(errorMessage);
	}
    m_Logger.Information("Window created.");

	// Create GPU device
	m_GpuDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, m_Configs->UseDeviceValidation, NULL);
	if (m_GpuDevice == nullptr) 
	{
        const char errorMessage[] = "GPU device creation failed.";
		// Logging::GetLogger()->Error(s_ServiceName, "Failed to create GPU device! SDL Error: %s", SDL_GetError());
        m_Logger.Fatal(errorMessage);
		return SdlCrashOut(errorMessage);
	}
    m_Logger.Information("GPU device created");
	
	// setup gpu window
	if (!SDL_ClaimWindowForGPUDevice(m_GpuDevice, m_Window))
	{
        static const char errorMessage[] = "GPU acceleration initialization failed.";
		m_Logger.Fatal(errorMessage);
        return SdlCrashOut(errorMessage);
	}
    m_Logger.Information("Accelerated window created.");
	
	SDL_SetWindowResizable(m_Window, false);
    SDL_ShowWindow(m_Window);

    // create a z-buffer
    SDL_GPUTextureCreateInfo depthBufferInfo {
        SDL_GPUTextureType::SDL_GPU_TEXTURETYPE_2D,
        SDL_GPUTextureFormat::SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
        SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        (uint32_t)m_Configs->WindowWidth,
        (uint32_t)m_Configs->WindowHeight,
        1,
        1
    };
    m_DepthBuffer = SDL_CreateGPUTexture(m_GpuDevice, &depthBufferInfo);

    if (m_DepthBuffer == nullptr)
    {
        static const char errorMessage[] = "Failed to create depth buffer.";
        m_Logger.Fatal(errorMessage);
        return SdlCrashOut(errorMessage);
    }

	return CallbackSuccess();
}

CallbackResult Engine::Core::Runtime::GraphicsLayer::BeginFrame()
{
    m_Logger.Verbose("Begin frame.");

    // create command buffer
    m_CommandBuffer = SDL_AcquireGPUCommandBuffer(m_GpuDevice);
    if (m_CommandBuffer == NULL)
    {
        const char errorMessage[] = "AcquireGPUCommandBuffer failed";
        m_Logger.Fatal(errorMessage);
        return SdlCrashOut(errorMessage);
    }

    // get a target texture
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(m_CommandBuffer, m_Window, &m_SwapchainTexture, nullptr, nullptr))
    {
        const char errorMessage[] = "WaitAndAcquireGPUSwapchainTexture failed.";
        m_Logger.Fatal(errorMessage);
        return SdlCrashOut(errorMessage);
    }

    // create a single pass to clear screen & clean depth buffer
    SDL_GPUColorTargetInfo colorTargetInfo = {0};
    colorTargetInfo.texture = m_SwapchainTexture;
    colorTargetInfo.clear_color = SDL_FColor{1.0f, 0.0f, 1.0f, 1.0f}; // ofc it should be magenta
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo depthStencilTargetInfo = {0};
    depthStencilTargetInfo.clear_depth = 1.0f;
    depthStencilTargetInfo.texture = m_DepthBuffer;
    depthStencilTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    depthStencilTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(m_CommandBuffer, &colorTargetInfo, 1, &depthStencilTargetInfo);
    SDL_EndGPURenderPass(pass);
    return CallbackSuccess();
}

CallbackResult Engine::Core::Runtime::GraphicsLayer::EndFrame()
{
    m_Logger.Verbose("End frame.");

	if (!SDL_SubmitGPUCommandBuffer(m_CommandBuffer))
    {
        const char errorMessage[] = "Failed to submit GPU command buffer.";
        m_Logger.Fatal(errorMessage);
        return SdlCrashOut(errorMessage);
    }
    
    m_CommandBuffer = nullptr;
    m_SwapchainTexture = nullptr;
    return CallbackSuccess();
}

GraphicsLayer::GraphicsLayer(const Configuration::ConfigurationProvider* configs, Logging::LoggerService* loggerService)
	: m_Configs(configs), m_Logger(loggerService->CreateLogger("GraphicsLayer")) {}

Engine::Core::Runtime::GraphicsLayer::~GraphicsLayer()
{
    // release auxilliary resources
    SDL_ReleaseGPUTexture(m_GpuDevice, m_DepthBuffer);

	// release gpu device
	SDL_ReleaseWindowFromGPUDevice(m_GpuDevice, m_Window);
	SDL_DestroyGPUDevice(m_GpuDevice);

	//Destroy window	
	SDL_DestroyWindow(m_Window);
	m_Window = NULL;

    // log
    m_Logger.Information("GPU resources cleaned up.");    
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

    SDL_GPUDepthStencilTargetInfo depthStencilTargetInfo = {0};
    depthStencilTargetInfo.texture = m_DepthBuffer;
    depthStencilTargetInfo.load_op = SDL_GPU_LOADOP_LOAD;
    depthStencilTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass *renderPass = SDL_BeginGPURenderPass(m_CommandBuffer, &colorTargetInfo, 1, &depthStencilTargetInfo);
    return renderPass;
}

void GraphicsLayer::CommitRenderPass(SDL_GPURenderPass* pass) 
{
    SDL_EndGPURenderPass(pass);
}
#include "EngineCore/Runtime/platform_access.h"
#include "EngineCore/Logging/logger_service.h"

#include <SDL3/SDL.h>

using namespace Engine::Core;
using namespace Engine::Core::Runtime;

static const char s_ServiceName[] = "PlatformAccess";
static const char s_SDLInitializationError[] = "SDL initialization failed, see logs for details.";
static const char s_GLInitializationError[] = "OpenGL initialization failed, see logs for details.";

bool Engine::Core::Runtime::PlatformAccess::InitializeSDL()
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

void Engine::Core::Runtime::PlatformAccess::BeginFrame()
{
	// clear screen?
    
}

void Engine::Core::Runtime::PlatformAccess::EndFrame()
{
	// finalize all the command buffers and swap?
}

PlatformAccess::PlatformAccess(const Configuration::ConfigurationProvider* configs)
	: m_Configs(configs) {}

Engine::Core::Runtime::PlatformAccess::~PlatformAccess()
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

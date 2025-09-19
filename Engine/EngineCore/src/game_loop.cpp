#include "EngineCore/Runtime/game_loop.h"
#include "SDL3/SDL_events.h"

using namespace Engine::Core::Runtime;

GameLoop::GameLoop() : m_ConfigurationProvider(), m_PlatformAccess(&m_ConfigurationProvider), m_RendererService(&m_PlatformAccess) {}

int GameLoop::Run() 
{
    if (!m_PlatformAccess.InitializeSDL())
    {
        return 1;
    }

	bool quit = false;
	SDL_Event e;
	while (!quit)
	{
		// Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			//User requests quit
			quit = e.type == SDL_EVENT_QUIT;
		}

		// begin update loop
		m_PlatformAccess.BeginFrame();
        // TODO: put the update logic here
		m_PlatformAccess.EndFrame();
	}

    return 0;
}
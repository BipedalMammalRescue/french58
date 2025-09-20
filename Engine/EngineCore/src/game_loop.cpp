#include "EngineCore/Runtime/game_loop.h"
#include "SDL3/SDL_events.h"

using namespace Engine::Core::Runtime;

GameLoop::GameLoop() : m_ConfigurationProvider(), m_PlatformAccess(&m_ConfigurationProvider) {}

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
        
        // TODO: regular update isn't implemented since we don't have dynamic behavior yet

        // TODO: render update, where each module is asked to create and submit render passes
        
        // last step in the update loop
		m_PlatformAccess.EndFrame();
	}

    return 0;
}
#include "EngineCore/Runtime/input_manager.h"
#include "SDL3/SDL_events.h"

using namespace Engine::Core::Runtime;

void InputManager::ProcessSdlEvents()
{
    m_QuitRequested = false;
    m_KeyDownEvents.clear();
    m_KeyUpEvents.clear();

    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        switch (e.type)
        {
        case SDL_EVENT_QUIT:
            m_QuitRequested = true;
            break;
        case SDL_EVENT_KEY_DOWN:
            m_KeyDownEvents.push_back(e.key.scancode);
            break;
        case SDL_EVENT_KEY_UP:
            m_KeyUpEvents.push_back(e.key.scancode);
            break;
        }
    }
}
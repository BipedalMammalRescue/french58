#pragma once

#include "SDL3/SDL_scancode.h"
#include <vector>
namespace Engine::Core::Runtime {

class InputManager 
{
private:
    friend class GameLoop;
    std::vector<SDL_Scancode> m_KeyDownEvents;
    std::vector<SDL_Scancode> m_KeyUpEvents;
    bool m_QuitRequested;

    void ProcessSdlEvents();

public:
    inline const std::vector<SDL_Scancode>* GetKeyDownEvents() const
    {
        return &m_KeyDownEvents;
    }

    inline const std::vector<SDL_Scancode>* GetKeyUpEvents() const 
    {
        return &m_KeyUpEvents;
    }
};

}
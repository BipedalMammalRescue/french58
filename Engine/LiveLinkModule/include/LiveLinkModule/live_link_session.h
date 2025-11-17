#pragma once

#include "EngineCore/Logging/logger.h"
#include "EngineCore/Runtime/service_table.h"
#include "SDL3_net/SDL_net.h"

namespace Engine::Extension::LiveLinkModule {

class LiveLinkSession
{
    Core::Runtime::ServiceTable* m_Services = nullptr;
    Core::Logging::Logger* m_Logger = nullptr;
    NET_StreamSocket* m_Socket = nullptr;
    int m_Slot = -1;
    
    int m_LeftoverLength = 0;
    char m_ReadBuffer[1024] = {0};

    bool m_IsActive = false;

public:
    void Initialize(Core::Runtime::ServiceTable* services, Core::Logging::Logger* logger, int slot)
    {
        m_Services = services;
        m_Logger = logger;
        m_Slot = slot;
        m_IsActive = false;
    }

    void Dispose();

    void ReadToExhaustion();

    int ProcessInputData(int validLength);

    inline bool IsActive() const
    {
        return m_IsActive;
    }

    inline void Activate(NET_StreamSocket* newSocket)
    {
        Dispose();
        m_Socket = newSocket;
        m_IsActive = true;
    }
};

}
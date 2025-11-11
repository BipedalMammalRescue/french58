#include "LiveLinkModule/live_link_session.h"
#include "SDL3_net/SDL_net.h"

using namespace Engine::Extension::LiveLinkModule;

enum class PacketType : unsigned char
{
    Invalid,
    Ping
};

void LiveLinkSession::Dispose()
{
    if (!m_IsActive)
        return;

    m_IsActive = false;
    NET_DestroyStreamSocket(m_Socket);
}

void LiveLinkSession::ReadToExhaustion()
{
    while (true)
    {
        int newRead = NET_ReadFromStreamSocket(m_Socket, m_ReadBuffer + m_LeftoverLength, sizeof (m_ReadBuffer) - m_LeftoverLength);

        if (newRead < 0)
        {
            m_Logger->Information("Connection slot {} closed.", m_Slot);
            Dispose();
            return;
        }

        if (newRead == 0)
            return;

        // handle the input
        int validLength = m_LeftoverLength + newRead;
        m_LeftoverLength = ProcessInputData(validLength);
    }
}

int LiveLinkSession::ProcessInputData(int validLength)
{
    int readLength = 0;

    while (readLength < validLength)
    {
        PacketType type = (PacketType)m_ReadBuffer[readLength];
        readLength++;

        switch (type)
        {
        case PacketType::Invalid:
            break;
        case PacketType::Ping:
            m_Logger->Information("Received ping from connection slot {}.", m_Slot);
            break;
        }
    }

    // data left
    if (readLength < validLength)
    {
        m_LeftoverLength = validLength - readLength;
        memcpy(m_ReadBuffer, m_ReadBuffer + readLength, m_LeftoverLength);
        return m_LeftoverLength;
    }

    return 0;
}
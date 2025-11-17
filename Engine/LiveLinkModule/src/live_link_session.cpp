#include "LiveLinkModule/live_link_session.h"

#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/asset_manager.h"

#include "SDL3_net/SDL_net.h"

using namespace Engine::Extension::LiveLinkModule;

enum class PacketType : unsigned char
{
    Invalid,
    Ping,
    HotReload
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

struct AssetReloadRequest
{
    Engine::Core::Pipeline::HashId Module;
    Engine::Core::Pipeline::HashId Type;
    Engine::Core::Pipeline::HashId Asset;
};

int LiveLinkSession::ProcessInputData(int validLength)
{
    int readLength = 0;
    bool keepReading = true;

    while (readLength < validLength && keepReading)
    {
        PacketType type = (PacketType)m_ReadBuffer[readLength];
        readLength++;

        switch (type)
        {
        case PacketType::Invalid:
            break;
        case PacketType::Ping:
            m_Logger->Information("Received ping from connection #{}.", m_Slot);
            break;
        case PacketType::HotReload:
            if (validLength - readLength < sizeof(AssetReloadRequest))
            {
                // stop here and revert the read status
                readLength --;
                keepReading = false;
                break;
            }
            else 
            {
                AssetReloadRequest* requestBody = (AssetReloadRequest*)&m_ReadBuffer[readLength];
                readLength += sizeof(AssetReloadRequest);

                m_Logger->Information("Received request from connection #{} to reload asset {} (module: {}, type: {}).", m_Slot, requestBody->Asset, requestBody->Module, requestBody->Type);
                m_Services->AssetManager->QueueAsset(requestBody->Module, requestBody->Type, requestBody->Asset);
                break;
            }
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
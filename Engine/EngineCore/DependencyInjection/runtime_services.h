#pragma once

#include <Platform/platform_access.h>
#include <Rendering/renderer_service.h>
#include <AssetManagement/asset_manager.h>
#include "configuration_provider.h"

namespace Engine::Core::DependencyInjection {

class RuntimeServices
{
  private:
    ConfigurationProvider m_Configurations;
    Platform::PlatformAccess m_PlatformAccess;
    Rendering::RendererService m_RendererService;
    AssetManagement::AssetManager m_AssetManager;

  public:
    inline const ConfigurationProvider *GetConfigurations()
    {
        return &m_Configurations;
    }

    inline Platform::PlatformAccess *GetPlatformAccess()
    {
        return &m_PlatformAccess;
    }

    inline Rendering::RendererService *GetRenderer()
    {
        return &m_RendererService;
    }

    inline AssetManagement::AssetManager* GetAssetManager() {
        return &m_AssetManager;
    }

    RuntimeServices() : m_Configurations(), m_PlatformAccess(&m_Configurations), m_RendererService(&m_PlatformAccess)
    {
    }
    ~RuntimeServices() = default;
};

} // namespace Engine::Core::DependencyInjection

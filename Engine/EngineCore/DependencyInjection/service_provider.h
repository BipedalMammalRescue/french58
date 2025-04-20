#pragma once

#include "Platform/platform_access.h"
#include "Rendering/renderer_service.h"
#include "configuration_provider.h"

namespace Engine {
namespace Core {
namespace DependencyInjection {

class RuntimeServices
{
  private:
    ConfigurationProvider m_Configurations;
    Platform::PlatformAccess m_PlatformAccess;
    Rendering::RendererService m_RendererService;

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

    RuntimeServices() : m_Configurations(), m_PlatformAccess(&m_Configurations), m_RendererService(&m_PlatformAccess)
    {
    }
    ~RuntimeServices() = default;
};

} // namespace DependencyInjection
} // namespace Core
} // namespace Engine

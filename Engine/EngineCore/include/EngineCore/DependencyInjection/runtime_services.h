#pragma once

#include "configuration_provider.h"
#include <EngineCore/AssetManagement/asset_manager.h>
#include <EngineCore/Platform/platform_access.h>
#include <EngineCore/Rendering/renderer_service.h>
#include <EngineCore/Runtime/memory_manager.h>

namespace Engine::Core::DependencyInjection {

class RuntimeServices
{
  private:
    ConfigurationProvider m_Configurations;
    Platform::PlatformAccess m_PlatformAccess;
    Rendering::RendererService m_RendererService;
    AssetManagement::AssetManager m_AssetManager;
    Runtime::MemoryManager m_MemoryManager;

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

    inline AssetManagement::AssetManager *GetAssetManager()
    {
        return &m_AssetManager;
    }

    inline Runtime::MemoryManager *GetMemoryManager()
    {
        return &m_MemoryManager;
    }

    RuntimeServices()
        : m_Configurations(), m_PlatformAccess(&m_Configurations), m_RendererService(&m_PlatformAccess),
          m_MemoryManager(Configuration::INITIAL_STACK_SIZE), m_AssetManager(&m_MemoryManager)
    {
    }
    ~RuntimeServices() = default;
};

} // namespace Engine::Core::DependencyInjection

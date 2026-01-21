#pragma once

#include "EngineCore/Configuration/configuration_provider.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Rendering/Lib/vk_mem_alloc.h"
#include "EngineCore/Rendering/Resources/device.h"
#include "EngineCore/Rendering/Resources/shader.h"
#include "EngineCore/Rendering/Resources/swapchain.h"
#include "EngineCore/Rendering/gpu_resource.h"
#include "EngineCore/Rendering/pipeline_setting.h"
#include "EngineCore/Rendering/render_target.h"
#include "EngineCore/Rendering/transfer_manager.h"
#include "EngineCore/Rendering/vertex_description.h"
#include "EngineCore/Runtime/crash_dump.h"

#include <cstdint>
#include <glm/fwd.hpp>
#include <vulkan/vulkan_core.h>

struct SDL_Window;

namespace Engine::Core::Logging {
class LoggerService;
}

namespace Engine::Core::Rendering {
class RenderThread;
} // namespace Engine::Core::Rendering

namespace Engine::Core::Runtime {

class GameLoop;

// Provides services used to draw stuff to screen.
// Current design this class IS the gameplay-thread representation of ALL graphics related
// resources.
class GraphicsLayer
{
private:
    friend class GameLoop;
    friend class Engine::Core::Rendering::RenderThread;

    const Configuration::ConfigurationProvider *m_Configs = nullptr;

    SDL_Window *m_Window = nullptr;
    Logging::Logger m_Logger;

    Rendering::Resources::Device m_Device;
    Rendering::Resources::Swapchain m_Swapchain;

    VmaAllocator m_VmaAllocator;
    Rendering::TransferManager m_TransferManager;

    struct
    {
        VkDescriptorPool GlobalDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout GlobalDescriptorLayout = VK_NULL_HANDLE;
        VkDescriptorSet GlobalDescriptorSet = VK_NULL_HANDLE;

        VkDescriptorSetLayout PerFlightDescLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout UboLayout = VK_NULL_HANDLE;

        VkPipelineLayout GlobalPipelineLayout = VK_NULL_HANDLE;
    } m_RenderResources;

    // built-in long-lasting resourcee
    Rendering::RenderTarget m_RenderTargets[2];
    Rendering::ColorAttachmentTarget m_OpaqueColor;
    Rendering::DepthAttachmentTarget m_OpaqueDepth;

    // managed resources
private:
    std::vector<uint32_t> m_GraphicsPipelineUpdates;
    std::vector<Rendering::Resources::Shader> m_Shaders;

    std::vector<uint32_t> m_GeometryUpdates;
    std::vector<Rendering::GpuGeometry> m_Geometries;

    // for game loop to directly control graphics behavior
private:
    CallbackResult InitializeSDL();

    CallbackResult BeginFrame();
    CallbackResult EndFrame();

    GraphicsLayer(const Configuration::ConfigurationProvider *configs,
                  Logging::LoggerService *loggerService);

public:
    inline Rendering::ColorAttachmentTarget GetOpqueColorBuffer() const
    {
        return m_OpaqueColor;
    }

    inline Rendering::DepthAttachmentTarget GetDepthAttachmentTarget() const
    {
        return m_OpaqueDepth;
    }

    uint32_t CompileShader(void *vertexCode, size_t vertShaderLength, void *fragmentCode,
                           size_t fragShaderLength,
                           const Rendering::VertexDescription *vertexSetting,
                           const Rendering::PipelineSetting *pipelineSetting,
                           const Rendering::ColorFormat *colorAttachments,
                           uint32_t colorAttachmentCount,
                           const Rendering::DepthPrecision *depthPrecision);

    inline std::optional<Rendering::Resources::StagingBuffer> CreateStagingBuffer(size_t size)
    {
        return m_TransferManager.CreateStagingBuffer(size);
    }

    inline void DestroyStagingBuffer(Rendering::Resources::StagingBuffer buffer)
    {
        m_TransferManager.DestroyStagingBuffer(buffer);
    }

    // TODO: eventually allow multi-buffer geometry
    uint32_t CreateGeometry(Rendering::Resources::StagingBuffer stagingBuffer,
                            Rendering::Transfer vertexBuffer, Rendering::Transfer indexBuffer,
                            uint32_t indexCount, Rendering::IndexType indexType);

public:
    ~GraphicsLayer();
};

} // namespace Engine::Core::Runtime
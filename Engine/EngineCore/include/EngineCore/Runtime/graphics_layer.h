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
class RenderPassExecutionContext;
} // namespace Engine::Core::Rendering

namespace Engine::Core::Runtime {

class GameLoop;

// Contains states and accesses to graphics related concepts, managed by the game loop.
class GraphicsLayer
{
    static constexpr uint32_t MaxFlight = 2;

private:
    friend class GameLoop;
    friend class Engine::Core::Rendering::RenderThread;
    friend class Rendering::RenderPassExecutionContext;

    // injected
    const Configuration::ConfigurationProvider *m_Configs = nullptr;

    // initialized
    SDL_Window *m_Window = nullptr;
    Logging::Logger m_Logger;

    Rendering::Resources::Device m_Device;
    Rendering::Resources::Swapchain m_Swapchain;

    VmaAllocator m_VmaAllocator;
    Rendering::TransferManager m_TransferManager;

    struct
    {
        VkCommandPool TransferCmdPool = VK_NULL_HANDLE;
        VkCommandBuffer TransferCmdBuffer = VK_NULL_HANDLE;

        // TODO: this shared fence is only useful on single-threaded, synchronous operations
        VkFence TransferFence = VK_NULL_HANDLE;
    } m_TransferUtils;

    struct
    {
        VkDescriptorPool GlobalDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout GlobalDescriptorLayout = VK_NULL_HANDLE;
        VkDescriptorSet GlobalDescriptorSet = VK_NULL_HANDLE;

        VkDescriptorSetLayout PerFlightDescLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout UboLayout = VK_NULL_HANDLE;

        VkPipelineLayout GlobalPipelineLayout = VK_NULL_HANDLE;
    } m_RenderResources;

    // TODO: render targets should be exclusively created on the render thread
    Rendering::RenderTarget CreateRenderTarget(Rendering::RenderTargetUsage usage,
                                               Rendering::RenderTargetSetting settings);

    bool CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, size_t *srcOffsets, size_t *dstOffsets,
                    size_t *lengths, size_t segmentCount);

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

    Rendering::StagingBuffer *CreateStagingBuffer(size_t size);

    inline void *MapStagingBuffer(Rendering::StagingBuffer *buffer)
    {
        void *dest = nullptr;
        vkMapMemory(m_Device.m_LogicalDevice, buffer->m_Memory, 0, buffer->m_Size, 0, &dest);
        return dest;
    }

    inline void UnmapStagingBuffer(Rendering::StagingBuffer *buffer)
    {
        vkUnmapMemory(m_Device.m_LogicalDevice, buffer->m_Memory);
    }

    // destroy a staging buffer, after which the data from said buffer will be unusable
    void DestroyStagingBuffer(Rendering::StagingBuffer *buffer);

    // geometry upload needs to support contextualize-index pattern, since those are data that would
    // be loaded in based on level
    uint32_t UploadGeometry(Rendering::StagingBuffer *stagingBuffer, size_t *vertexBufferOffsets,
                            size_t *vertexBufferLengths, uint32_t vertexBufferCount,
                            size_t indexBufferOffset, size_t indexBufferLength,
                            Rendering::IndexType indexType, uint32_t indexCount);

public:
    ~GraphicsLayer();
};

} // namespace Engine::Core::Runtime
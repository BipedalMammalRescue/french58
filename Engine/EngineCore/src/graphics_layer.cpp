#include "EngineCore/Runtime/graphics_layer.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Logging/logger_service.h"
#include "EngineCore/Rendering/Resources/device.h"
#include "EngineCore/Rendering/Resources/geometry.h"
#include "EngineCore/Rendering/Resources/shader.h"
#include "EngineCore/Rendering/gpu_resource.h"
#include "EngineCore/Rendering/render_target.h"
#include "EngineCore/Rendering/transfer_manager.h"
#include "EngineCore/Rendering/vertex_description.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/service_table.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_stdinc.h"

#include "Rendering/common.h"

#include <cstdint>
#include <vulkan/vulkan_core.h>

using namespace Engine::Core;
using namespace Engine::Core::Runtime;
using namespace Engine::Core::Rendering;

static const char *LogChannels[] = {"GraphicsLayer"};

GraphicsLayer::GraphicsLayer(const Configuration::ConfigurationProvider *configs,
                             Logging::LoggerService *loggerService)
    : m_Configs(configs), m_Logger(loggerService->CreateLogger("GraphicsLayer")),
      m_RenderThread(&m_Device, &m_Swapchain)
{
}

CallbackResult GraphicsLayer::Initialize(ServiceTable *services)
{
    // Create window
    m_Window = SDL_CreateWindow("Foobar Game", m_Configs->WindowWidth, m_Configs->WindowHeight, 0);
    if (m_Window == nullptr)
        return Crash(__FILE__, __LINE__,
                     std::string("Failed to create SDL window, error: ") + SDL_GetError());
    m_Logger.Information("Window created.");

    // initialize device
    CallbackResult deviceInitResult =
        m_Device.Initialize(&m_Logger, m_Configs->EnableVulkanValidationLayers, m_Window);
    if (deviceInitResult.has_value())
        return deviceInitResult;

    // create the swapchain
    if (!m_Swapchain.Initialize(&m_Logger, m_Configs, m_Device.m_LogicalDevice,
                                m_Device.m_PhysicalDevice, m_Device.m_Surface,
                                m_Device.m_GraphicsQueueSelection.queueIndex,
                                m_Device.m_GraphicsQueueSelection.presentQueueIndex))
        return Runtime::Crash(__FILE__, __LINE__, "Failed to initialize swapchain.");

    // create main thread GPU memory management (asset pipeline currently runs on the gameplay
    // thread, and it doesn't make much sense to share it with the rendering thread sicne memory
    // there are made to be constantly churned)
    VmaAllocatorCreateInfo allocatorCreateInfo = {
        .flags = 0,
        .physicalDevice = m_Device.m_PhysicalDevice,
        .device = m_Device.m_LogicalDevice,
        .vulkanApiVersion = m_Device.m_Version,
    };
    CHECK_VULKAN(vmaCreateAllocator(&allocatorCreateInfo, &m_VmaAllocator),
                 "Failed to create VMA allocator.");
    m_TransferManager.Initialize(&m_Logger, m_VmaAllocator, m_Device.m_LogicalDevice,
                                 m_Device.m_TransferQueue,
                                 m_Device.m_TransferQueueSelection.queueFamilyIndex);

    // bindless setup (and pretty much all the metadata needed for pipeline layout)
    {
        static constexpr VkDescriptorPoolSize globalPoolSizes[] = {
            (VkDescriptorPoolSize){
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = MaxStorageBuffers,
            },
            (VkDescriptorPoolSize){
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = MaxImageSamplers,
            },
        };
        static constexpr VkDescriptorPoolCreateInfo globalPoolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
            .maxSets = 1,
            .poolSizeCount = SDL_arraysize(globalPoolSizes),
            .pPoolSizes = globalPoolSizes,
        };
        CHECK_VULKAN(vkCreateDescriptorPool(m_Device.m_LogicalDevice, &globalPoolInfo, nullptr,
                                            &m_RenderResources.GlobalDescriptorPool),
                     "Failed to allocate bindless descriptor pool.");

        static constexpr VkDescriptorSetLayoutBinding globalSetBindings[] = {
            (VkDescriptorSetLayoutBinding){
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 65536,
                .stageFlags = VK_SHADER_STAGE_ALL,
            },
            (VkDescriptorSetLayoutBinding){
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 65536,
                .stageFlags = VK_SHADER_STAGE_ALL,
            },
            (VkDescriptorSetLayoutBinding){
                .binding = 2,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = 65536,
                .stageFlags = VK_SHADER_STAGE_ALL,
            },
        };
        VkDescriptorSetLayoutCreateInfo globalSetLayoutCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
            .bindingCount = SDL_arraysize(globalSetBindings),
            .pBindings = globalSetBindings,
        };
        CHECK_VULKAN(vkCreateDescriptorSetLayout(m_Device.m_LogicalDevice,
                                                 &globalSetLayoutCreateInfo, nullptr,
                                                 &m_RenderResources.GlobalDescriptorLayout),
                     "Failed to create bindless descriptor set layout.");

        VkDescriptorSetAllocateInfo globalsetAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = m_RenderResources.GlobalDescriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &m_RenderResources.GlobalDescriptorLayout,
        };
        CHECK_VULKAN(vkAllocateDescriptorSets(m_Device.m_LogicalDevice, &globalsetAllocateInfo,
                                              &m_RenderResources.GlobalDescriptorSet),
                     "Failed to allocate bindless descriptor set.");

        static constexpr VkDescriptorSetLayoutBinding perFlightSetBindings[] = {
            (VkDescriptorSetLayoutBinding){
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 65536,
                .stageFlags = VK_SHADER_STAGE_ALL,
            },
        };
        static constexpr VkDescriptorSetLayoutCreateInfo perFlightSetLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
            .bindingCount = SDL_arraysize(perFlightSetBindings),
            .pBindings = perFlightSetBindings,
        };
        CHECK_VULKAN(vkCreateDescriptorSetLayout(m_Device.m_LogicalDevice,
                                                 &perFlightSetLayoutCreateInfo, nullptr,
                                                 &m_RenderResources.PerFlightDescLayout),
                     "Failed to create render-graph related descriptor set layout.");

        static constexpr VkDescriptorSetLayoutBinding uniformBufferBindings[] = {
            (VkDescriptorSetLayoutBinding){
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_ALL,
            }};
        static constexpr VkDescriptorSetLayoutCreateInfo uniformBufferLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .bindingCount = SDL_arraysize(uniformBufferBindings),
            .pBindings = uniformBufferBindings,
        };
        CHECK_VULKAN(vkCreateDescriptorSetLayout(m_Device.m_LogicalDevice,
                                                 &uniformBufferLayoutCreateInfo, nullptr,
                                                 &m_RenderResources.UboLayout),
                     "Failed to create ubo descriptor set layout.");

        // now with the layouts created this is enough information to create a pipeline layout

        VkPushConstantRange pushConstant[] = {
            (VkPushConstantRange){
                .stageFlags = VK_SHADER_STAGE_ALL,
                .offset = 0,
                .size = Rendering::PushConstantSize,
            },
        };
        VkDescriptorSetLayout layouts[]{
            m_RenderResources.GlobalDescriptorLayout,
            m_RenderResources.PerFlightDescLayout,
            m_RenderResources.UboLayout,
        };
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = SDL_arraysize(layouts),
            .pSetLayouts = layouts,
            .pushConstantRangeCount = SDL_arraysize(layouts),
            .pPushConstantRanges = pushConstant,
        };
        CHECK_VULKAN(vkCreatePipelineLayout(m_Device.m_LogicalDevice, &pipelineLayoutInfo, nullptr,
                                            &m_RenderResources.GlobalPipelineLayout),
                     "Failed to create global graphics pipeline layout.");
    }

    // initialize render thread
    CallbackResult renderThreadInitResult = m_RenderThread.MtStart(
        services, m_RenderResources.GlobalPipelineLayout, m_RenderResources.PerFlightDescLayout);
    if (renderThreadInitResult.has_value())
        return renderThreadInitResult;

    return CallbackSuccess();
}

uint32_t GraphicsLayer::CompileShader(void *vertexCode, size_t vertShaderLength, void *fragmentCode,
                                      size_t fragShaderLength,
                                      const Rendering::VertexDescription *vertexSetting,
                                      const Rendering::PipelineSetting *pipelineSetting,
                                      const Rendering::ColorFormat *colorAttachments,
                                      uint32_t colorAttachmentCount,
                                      const Rendering::DepthPrecision *depthPrecision)
{
    Rendering::Resources::Shader newShader = {};
    VkResult result =
        newShader.Initialize(m_Device.m_LogicalDevice, m_Swapchain.m_Format, &m_Logger,
                             {
                                 m_RenderResources.GlobalPipelineLayout,
                                 vertexCode,
                                 vertShaderLength,
                                 fragmentCode,
                                 fragShaderLength,
                                 vertexSetting,
                                 pipelineSetting,
                                 colorAttachments,
                                 colorAttachmentCount,
                                 depthPrecision,
                             });

    if (result != VK_SUCCESS)
    {
        m_Logger.Error("Failed to compile shader, error: {}", Log(result));
        return UINT32_MAX;
    }

    uint32_t newId = m_Shaders.size();
    m_Shaders.push_back(newShader);
    m_GraphicsPipelineUpdates.push_back(newId);
    return newId;
}

uint32_t GraphicsLayer::CreateGeometry(Rendering::Resources::StagingBuffer stagingBuffer,
                                       Rendering::Transfer vertexBuffer,
                                       Rendering::Transfer indexBuffer, uint32_t indexCount,
                                       Rendering::IndexType indexType)
{
    Rendering::Resources::Geometry newGeometry;

    VkIndexType vkIndexType = VK_INDEX_TYPE_UINT32;

    switch (indexType)
    {
    case IndexType::Ubyte:
        vkIndexType = VK_INDEX_TYPE_UINT8;
        break;
    case IndexType::Ushort:
        vkIndexType = VK_INDEX_TYPE_UINT16;
        break;
    case IndexType::Uint:
        vkIndexType = VK_INDEX_TYPE_UINT32;
        break;
    default:
        m_Logger.Error("Failed to create geometry, invalid index type: {}",
                       static_cast<uint32_t>(indexType));
        return UINT32_MAX;
    }

    newGeometry.Initialize(&m_TransferManager, {.StagingBuffer = stagingBuffer,
                                                .VertexBuffer = vertexBuffer,
                                                .IndexBuffer = indexBuffer,
                                                .IndexCount = indexCount,
                                                .IndexType = vkIndexType});

    // register
    uint32_t newId = m_Geometries.size();
    m_Geometries.push_back(newGeometry);
    m_GeometryUpdates.push_back(newId);
    return newId;
}

Engine::Core::Runtime::CallbackResult Engine::Core::Runtime::GraphicsLayer::BeginFrame()
{
    // wait for all transfer operations to finish
    m_TransferManager.Join();
    return m_RenderThread.MtUpdate(
        {
            m_Shaders.data(),
            m_Shaders.size(),
            m_GraphicsPipelineUpdates.data(),
            m_GraphicsPipelineUpdates.size(),
        },
        {
            m_Geometries.data(),
            m_Geometries.size(),
            m_GeometryUpdates.data(),
            m_GeometryUpdates.size(),
        },
        false);
}

#include "EngineCore/Runtime/graphics_layer.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Logging/logger_service.h"
#include "EngineCore/Rendering/Resources/device.h"
#include "EngineCore/Rendering/Resources/shader.h"
#include "EngineCore/Rendering/gpu_resource.h"
#include "EngineCore/Rendering/render_target.h"
#include "EngineCore/Rendering/vertex_description.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_stdinc.h"

#include "Rendering/common.h"

#include <cmath>
#include <cstdint>
#include <vulkan/vulkan_core.h>

using namespace Engine::Core;
using namespace Engine::Core::Runtime;
using namespace Engine::Core::Rendering;

static const char *LogChannels[] = {"GraphicsLayer"};

CallbackResult GraphicsLayer::InitializeSDL()
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
                                m_Device.m_GraphicsQueueIndex, m_Device.m_PresentQueueIndex))
        return Runtime::Crash(__FILE__, __LINE__, "Failed to initialize swapchain.");

    // create transfer utils
    {
        VkCommandPoolCreateInfo cmdPoolInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                     VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = m_Device.m_TransferQueueIndex,
        };
        CHECK_VULKAN(vkCreateCommandPool(m_Device.m_LogicalDevice, &cmdPoolInfo, nullptr,
                                         &m_TransferUtils.TransferCmdPool),
                     "Failed to create Vulkan transfer command pool.");

        VkCommandBufferAllocateInfo cmdBufferInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = m_TransferUtils.TransferCmdPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        CHECK_VULKAN(vkAllocateCommandBuffers(m_Device.m_LogicalDevice, &cmdBufferInfo,
                                              &m_TransferUtils.TransferCmdBuffer),
                     "Failed to allocate transfer command buffer.");

        VkFenceCreateInfo fenceInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
        };
        CHECK_VULKAN(vkCreateFence(m_Device.m_LogicalDevice, &fenceInfo, nullptr,
                                   &m_TransferUtils.TransferFence),
                     "Failed to create transfer fence.");
    }

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

    // set up the critical path rendering targets (THESE ARE NOT BACKED BY MEMORY JUST YET)
    {
        m_OpaqueColor.m_Id = 0;
        m_RenderTargets[0] =
            CreateRenderTarget(RenderTargetUsage::ColorTarget,
                               (RenderTargetSetting){
                                   .Image =
                                       {
                                           .ColorTarget =
                                               {
                                                   .ScaleToSwapchain = true,
                                                   .Dimensions = {.Relative = {1.0f, 1.0f}},
                                                   .ColorFormat = ColorFormat::UseSwapchain,
                                               },
                                       },
                                   .Sampler = Rendering::SamplerType::None,
                               });

        m_OpaqueDepth.m_Id = 1;
        m_RenderTargets[1] =
            CreateRenderTarget(Rendering::RenderTargetUsage::DepthBuffer,
                               (RenderTargetSetting){
                                   .Image =
                                       {
                                           .ColorTarget =
                                               {
                                                   .ScaleToSwapchain = true,
                                                   .Dimensions = {.Relative = {1.0f, 1.0f}},
                                                   .ColorFormat = ColorFormat::UseSwapchain,
                                               },
                                       },
                                   .Sampler = Rendering::SamplerType::None,
                               });
    }

    return CallbackSuccess();
}

RenderTarget GraphicsLayer::CreateRenderTarget(Rendering::RenderTargetUsage usage,
                                               Rendering::RenderTargetSetting settings)
{
    VkFormat format;
    uint32_t width;
    uint32_t height;

    // TODO: when do I need to change the tiling setting?
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkImageUsageFlags gpuUsage;
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    switch (usage)
    {
    case RenderTargetUsage::ColorTarget: {
        switch (settings.Image.ColorTarget.ColorFormat)
        {
        case ColorFormat::UseSwapchain:
            format = m_Swapchain.m_Format;
            break;
        }

        if (settings.Image.ColorTarget.ScaleToSwapchain)
        {
            width = static_cast<uint32_t>(
                round(settings.Image.ColorTarget.Dimensions.Relative.WidthScale *
                      m_Swapchain.m_Dimensions.width));
            height = static_cast<uint32_t>(
                round(settings.Image.ColorTarget.Dimensions.Relative.HeightScale *
                      m_Swapchain.m_Dimensions.height));
        }
        else
        {
            width = settings.Image.ColorTarget.Dimensions.Absolute.Width;
            height = settings.Image.ColorTarget.Dimensions.Absolute.Height;
        }

        gpuUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    break;
    case RenderTargetUsage::DepthBuffer: {
        switch (settings.Image.DepthBuffer.Precision)
        {
        case Engine::Core::Rendering::DepthPrecision::D32:
            format = VK_FORMAT_D32_SFLOAT;
            break;
        }

        if (settings.Image.DepthBuffer.ScaleToSwapchain)
        {
            width = static_cast<uint32_t>(
                round(settings.Image.DepthBuffer.Dimensions.Relative.WidthScale *
                      m_Swapchain.m_Dimensions.width));
            height = static_cast<uint32_t>(
                round(settings.Image.DepthBuffer.Dimensions.Relative.HeightScale *
                      m_Swapchain.m_Dimensions.height));
        }
        else
        {
            width = settings.Image.DepthBuffer.Dimensions.Absolute.Width;
            height = settings.Image.DepthBuffer.Dimensions.Absolute.Height;
        }
    }
    break;
    }

    if (settings.Sampler > Rendering::SamplerType::None)
    {
        gpuUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }

    GpuImage image = CreateImage(m_Device.m_LogicalDevice, m_Device.m_PhysicalDevice, width, height,
                                 format, tiling, gpuUsage, properties);

    return {
        .Setting = settings,
        .Image = image,
        .View = VK_NULL_HANDLE,
    };
}

GraphicsLayer::GraphicsLayer(const Configuration::ConfigurationProvider *configs,
                             Logging::LoggerService *loggerService)
    : m_Configs(configs), m_Logger(loggerService->CreateLogger("GraphicsLayer"))
{
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
    if (!newShader.Initialize(m_Device.m_LogicalDevice, m_Swapchain.m_Format, &m_Logger,
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
                              }))
        return UINT32_MAX;

    uint32_t newId = m_Shaders.size();
    m_Shaders.push_back(newShader);
    m_GraphicsPipelineUpdates.push_back(newId);
    return newId;
}

StagingBuffer *GraphicsLayer::CreateStagingBuffer(size_t size)
{
    // TODO: allocate buffers like this from a free list
    StagingBuffer *newBuffer = new StagingBuffer;
    newBuffer->m_Size = size;

    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    if (vkCreateBuffer(m_Device.m_LogicalDevice, &bufferInfo, nullptr, &newBuffer->m_Buffer) !=
        VK_SUCCESS)
    {
        m_Logger.Error("Failed to create staging buffer.");
        delete newBuffer;
        return nullptr;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_Device.m_LogicalDevice, newBuffer->m_Buffer, &memRequirements);

    // TODO: allocation needs to be rewritten, cache mem type/index
    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits,
                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                          m_Device.m_PhysicalDevice),
    };

    if (vkAllocateMemory(m_Device.m_LogicalDevice, &allocInfo, nullptr, &newBuffer->m_Memory) !=
        VK_SUCCESS)
    {
        m_Logger.Error("Failed to allocate memory for new staging buffer.");
        delete newBuffer;
        return nullptr;
    }

    vkBindBufferMemory(m_Device.m_LogicalDevice, newBuffer->m_Buffer, newBuffer->m_Memory, 0);
    return newBuffer;
}

void GraphicsLayer::DestroyStagingBuffer(Rendering::StagingBuffer *buffer)
{
    vkDestroyBuffer(m_Device.m_LogicalDevice, buffer->m_Buffer, nullptr);
    vkFreeMemory(m_Device.m_LogicalDevice, buffer->m_Memory, nullptr);
}

bool GraphicsLayer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, size_t *srcOffsets,
                               size_t *dstOffsets, size_t *lengths, size_t segmentCount)
{
    // wait for the previous operation to be done
    vkWaitForFences(m_Device.m_LogicalDevice, 1, &m_TransferUtils.TransferFence, VK_TRUE,
                    UINT64_MAX);

    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };
    if (vkBeginCommandBuffer(m_TransferUtils.TransferCmdBuffer, &beginInfo) != VK_SUCCESS)
    {
        m_Logger.Error("Failed to begin transfer command buffer.");
        return false;
    }

    for (size_t i = 0; i < segmentCount; i++)
    {
        VkBufferCopy copyRegion{
            .srcOffset = srcOffsets[i],
            .dstOffset = dstOffsets[i],
            .size = lengths[i],
        };
        vkCmdCopyBuffer(m_TransferUtils.TransferCmdBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    }

    vkEndCommandBuffer(m_TransferUtils.TransferCmdBuffer);

    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &m_TransferUtils.TransferCmdBuffer,
    };
    vkQueueSubmit(m_Device.m_TransferQueue, 1, &submitInfo, m_TransferUtils.TransferFence);
    return true;
}

uint32_t GraphicsLayer::UploadGeometry(Rendering::StagingBuffer *stagingBuffer,
                                       size_t *vertexBufferOffsets, size_t *vertexBufferLengths,
                                       uint32_t vertexBufferCount, size_t indexBufferOffset,
                                       size_t indexBufferLength, Rendering::IndexType indexType,
                                       uint32_t indexCount)
{
    GpuGeometry geometry{
        .VertexBufferCount = vertexBufferCount,
        .IndexCount = indexCount,
        .IndexType = VK_INDEX_TYPE_NONE_KHR,
    };

    switch (indexType)
    {
    case IndexType::Ubyte:
        geometry.IndexType = VK_INDEX_TYPE_UINT8;
        break;
    case IndexType::Ushort:
        geometry.IndexType = VK_INDEX_TYPE_UINT16;
        break;
    case IndexType::Uint:
        geometry.IndexType = VK_INDEX_TYPE_UINT32;
        break;
    }

    if (vertexBufferCount > MaxVertexBufferBindings)
    {
        m_Logger.Error("Cannot create geometry with more than {} vertex buffer bindings.",
                       MaxVertexBufferBindings);
        return UINT32_MAX;
    }

    // calculate needed buffer size in total
    size_t bufferSize = indexBufferLength;
    for (size_t i = 0; i < vertexBufferCount; i++)
    {
        bufferSize += vertexBufferLengths[i];
    }

    // create a on-board buffer to hold all the data
    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = bufferSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    if (vkCreateBuffer(m_Device.m_LogicalDevice, &bufferInfo, nullptr, &geometry.Buffer) !=
        VK_SUCCESS)
    {
        m_Logger.Error("Failed to create buffer for geometry.");
        return UINT32_MAX;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_Device.m_LogicalDevice, geometry.Buffer, &memRequirements);

    // TODO: cache memory type
    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex =
            FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                           m_Device.m_PhysicalDevice),
    };

    if (vkAllocateMemory(m_Device.m_LogicalDevice, &allocInfo, nullptr, &geometry.Memory) !=
        VK_SUCCESS)
    {
        m_Logger.Error("Failed to allocate memory for geometry.");
        vkDestroyBuffer(m_Device.m_LogicalDevice, geometry.Buffer, nullptr);
        return UINT32_MAX;
    }

    vkBindBufferMemory(m_Device.m_LogicalDevice, geometry.Buffer, geometry.Memory, 0);

    // prepare for the copy
    size_t srcOffsets[MaxVertexBufferBindings + 1];
    size_t dstOffsets[MaxVertexBufferBindings + 1];
    size_t lengths[MaxVertexBufferBindings + 1];

    size_t totalVertexSize = 0;
    for (size_t i = 0; i < vertexBufferCount; i++)
    {
        srcOffsets[i] = vertexBufferOffsets[i];
        dstOffsets[i] = totalVertexSize;
        lengths[i] = vertexBufferLengths[i];
        totalVertexSize += vertexBufferLengths[i];
        geometry.VertexBufferOffsets[i] = dstOffsets[i];
    }
    srcOffsets[vertexBufferCount] = indexBufferOffset;
    dstOffsets[vertexBufferCount] = totalVertexSize;
    lengths[vertexBufferCount] = indexBufferLength;
    geometry.IndexBufferOffset = totalVertexSize;

    // copy all segments of the staging buffer into the geometry buffer
    CopyBuffer(stagingBuffer->m_Buffer, geometry.Buffer, srcOffsets, dstOffsets, lengths,
               vertexBufferCount + 1);

    // register
    uint32_t newId = m_Geometries.size();
    m_Geometries.push_back(geometry);
    m_GeometryUpdates.push_back(newId);
    return newId;
}
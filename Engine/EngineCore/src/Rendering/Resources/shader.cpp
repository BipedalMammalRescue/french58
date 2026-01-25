#include "EngineCore/Rendering/Resources/shader.h"
#include "EngineCore/Rendering/Resources/geometry.h"
#include "SDL3/SDL_stdinc.h"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

using namespace Engine::Core::Rendering;
using namespace Engine::Core;

static void TranslateVertexAttributes(const VertexDescription *vertexSetting,
                                      VkVertexInputAttributeDescription *destAttr,
                                      VkVertexInputBindingDescription *destBinding)
{
    size_t attrOffset = 0;

    for (uint32_t i = 0; i < vertexSetting->BindingCount; i++)
    {
        uint32_t stride = 0;

        for (uint32_t attrId = 0; attrId < vertexSetting->Bindings[i].AttributeCount; attrId++)
        {
            uint32_t offset = stride;
            VkFormat format = VK_FORMAT_UNDEFINED;

            switch (vertexSetting->Bindings[i].Attributes[attrId])
            {
            case VertexDataTypes::Float:
                stride += sizeof(float);
                format = VK_FORMAT_R32_SFLOAT;
                break;
            case VertexDataTypes::Vector2:
                stride += sizeof(glm::vec2);
                format = VK_FORMAT_R32G32_SFLOAT;
                break;
            case VertexDataTypes::Vector3:
                stride += sizeof(glm::vec3);
                format = VK_FORMAT_R32G32B32_SFLOAT;
                break;
            case VertexDataTypes::Vector4:
                stride += sizeof(glm::vec4);
                format = VK_FORMAT_R32G32B32A32_SFLOAT;
                break;
            }

            destAttr[attrOffset] = {
                .location = attrId,
                .binding = i,
                .format = format,
                .offset = offset,
            };

            attrOffset++;
        }

        destBinding[i] = {
            .binding = i,
            .stride = stride,

            // TODO: the engine doesn't support instanced rendering rn, will need to fix it here
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };
    }
}

class TemporaryShaderModule
{
private:
    VkDevice m_Device = VK_NULL_HANDLE;
    VkShaderModule m_Shader = VK_NULL_HANDLE;
    VkResult m_InitResult = VK_ERROR_UNKNOWN;

public:
    TemporaryShaderModule(void *code, size_t length, VkDevice device) : m_Device(device)
    {
        VkShaderModuleCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = length,
            .pCode = reinterpret_cast<const uint32_t *>(code),
        };

        m_InitResult = vkCreateShaderModule(device, &createInfo, nullptr, &m_Shader);
    }

    inline VkResult Success() const
    {
        return m_InitResult;
    }

    inline VkShaderModule Get() const
    {
        return m_Shader;
    }

    ~TemporaryShaderModule()
    {
        if (m_Device != VK_NULL_HANDLE && m_Shader != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(m_Device, m_Shader, nullptr);
        }
    }
};

VkResult Resources::Shader::Initialize(VkDevice device, VkFormat swapchainFormat,
                                       Logging::Logger *logger, ShaderCreationInfo createInfo)
{
    if (createInfo.VertexSetting->BindingCount > MaxVertexBufferBindings)
    {
        logger->Error("Shader contains {} bindings, where only {} is allowed.",
                      createInfo.VertexSetting->BindingCount, MaxVertexBufferBindings);
        return VK_ERROR_UNKNOWN;
    }

    uint32_t totalAttributeCount = 0;
    for (auto *binding = createInfo.VertexSetting->Bindings;
         binding < createInfo.VertexSetting->Bindings + createInfo.VertexSetting->BindingCount;
         binding++)
    {
        totalAttributeCount += binding->AttributeCount;
    }

    if (totalAttributeCount > MaxVertexBufferAttributes)
    {
        logger->Error("Shader contains {} total attributes, where only {} is allowed.",
                      totalAttributeCount, MaxVertexBufferAttributes);
        return VK_ERROR_UNKNOWN;
    }

    VkVertexInputBindingDescription bindings[MaxVertexBufferBindings];
    VkVertexInputAttributeDescription attributes[MaxVertexBufferAttributes];
    TranslateVertexAttributes(createInfo.VertexSetting, attributes, bindings);

    // create the shader modules
    TemporaryShaderModule vertShader(createInfo.VertexCode, createInfo.VertShaderLength, device);
    if (vertShader.Success() != VK_SUCCESS)
    {
        logger->Error("Failed to compile vertex shader.");
        return vertShader.Success();
    }

    TemporaryShaderModule fragShader(createInfo.FragmentCode, createInfo.FragShaderLength, device);
    if (fragShader.Success() != VK_SUCCESS)
    {
        logger->Error("Failed to compile fragment shaders.");
        return fragShader.Success();
    }

    // create the pipeline
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShader.Get(),
        .pName = "main",
    };
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShader.Get(),
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = SDL_arraysize(dynamicStates),
        .pDynamicStates = dynamicStates,
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = createInfo.VertexSetting->BindingCount,
        .pVertexBindingDescriptions = bindings,
        .vertexAttributeDescriptionCount = totalAttributeCount,
        .pVertexAttributeDescriptions = attributes,
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = nullptr,
        .scissorCount = 1,
        .pScissors = nullptr,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampling{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    // TODO: there's been no knowledge how to change this yet
    VkPipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo colorBlending{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY, // TODO: wtf is this?
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f} // optional
    };

    // infer the color formats
    std::unique_ptr<VkFormat[]> formats =
        std::make_unique<VkFormat[]>(createInfo.ColorAttachmentCount);
    for (size_t i = 0; i < createInfo.ColorAttachmentCount; i++)
    {
        formats[i] = swapchainFormat;
        switch (createInfo.ColorAttachments[i])
        {
        case ColorFormat::UseSwapchain:
            break;
        }
    }

    // infer the depth format
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    if (createInfo.DepthPrecision != nullptr)
    {
        switch (*createInfo.DepthPrecision)
        {
        case DepthPrecision::D32:
            depthFormat = VK_FORMAT_D32_SFLOAT;
            break;
        }
    }

    // this one is rather proper
    VkPipelineRenderingCreateInfo renderingCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .pNext = nullptr,
        .viewMask = 0,
        .colorAttachmentCount = createInfo.ColorAttachmentCount,
        .pColorAttachmentFormats = formats.get(),
        .depthAttachmentFormat = depthFormat,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
    };

    // TODO: rn we hard-code that depth-test is always enabled with depth-write
    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .depthTestEnable = createInfo.DepthPrecision ? VK_TRUE : VK_FALSE,
        .depthWriteEnable = createInfo.DepthPrecision ? VK_TRUE : VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
    };

    // create the pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,

        // this should be controlled by the engine (feature enabling)
        .pNext = &renderingCreateInfo,

        // this is managed by the engine too (engine only uses two stages)
        .stageCount = 2,
        .pStages = shaderStages,

        // this should be controlled by the client (data format)
        .pVertexInputState = &vertexInputInfo,

        // this should be managed by the engine (engine only allows triangle lists)
        .pInputAssemblyState = &inputAssembly,

        // controlled by the engine (only use dynamic state for viewport and scissor)
        .pViewportState = &viewportState,

        // should be exposed to the client, this is closely related to the desired behavior
        .pRasterizationState = &rasterizer,

        // probably controlled by the engine? multisampling feels like something that should be
        // configured together
        .pMultisampleState = &multisampling,

        // managed by the depth stencil parameters
        .pDepthStencilState = &depthStencilCreateInfo,

        // maybe controlled by the client? no need for it yet
        .pColorBlendState = &colorBlending,

        // controlled by the engine all of the below
        .pDynamicState = &dynamicState,
        .layout = createInfo.Layout,
        .renderPass = VK_NULL_HANDLE,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE, // Optional
        .basePipelineIndex = -1               // Optional
    };

    VkPipeline newPipe = VK_NULL_HANDLE;
    return vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipe);
}
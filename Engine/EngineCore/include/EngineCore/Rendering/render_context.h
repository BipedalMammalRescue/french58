#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Rendering/render_target.h"
#include "EngineCore/Runtime/graphics_layer.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Engine::Core::Runtime {
class GraphicsLayer;
}

namespace Engine::Core::Logging {
class Logger;
}

namespace Engine::Core::Rendering {

class RenderPassExecutionContext
{
private:
    friend class RenderStageExecutionContext;
    Logging::Logger *m_Logger;
    Rendering::RenderThread *m_RenderThread;
    VkCommandBuffer *m_CommandBuffer;

    Pipeline::HashId m_CurrentModule;

public:
    void SetPipeline(uint32_t pipelineId);
    void Draw(uint32_t geometryId, void *pushConstantData, size_t pushConstantSize,
              uint32_t uniformId);
};

class RenderStageExecutionContext
{
public:
    RenderPassExecutionContext BeginRenderPass(OutputColorTarget *colorTargets,
                                               size_t colorTargetCount,
                                               std::optional<OutputDepthTarget> depthTarget);
};

class RenderPassConfigurator
{
private:
    friend class RenderSetupContext;

public:
    OutputColorTarget WriteTo(ColorAttachmentTarget target);
    OutputDepthTarget WriteTo(DepthAttachmentTarget target);
};

// TODO: implement setup and execution here
class RenderSetupContext
{
private:
    friend class RenderThread;

    RenderPassConfigurator m_Configurator;

    struct BuiltLambda
    {
        size_t StorageOffset;
        void (*Lambda)(void *);
    };

    std::vector<uint8_t> m_Storage;
    std::vector<BuiltLambda> m_Lambdas;

    inline void Reset()
    {
        m_Storage.clear();
        m_Lambdas.clear();
    }

public:
    template <typename TData, typename TSetupFunc, typename TExeFunc>
    void AddRenderStage(TSetupFunc setupLambda, TExeFunc executeLambda)
    {
        struct Carrier
        {
            TData SetupData;
            TExeFunc Function;

            static void LambdaCaller(void *data)
            {
                Carrier *carrier = static_cast<Carrier *>(data);
                carrier->Function(carrier->SetupData);
            }
        } carrier{
            .SetupData = setupLambda(&m_Configurator),
            .Function = executeLambda,
        };

        // store the entire lambda and the built parameters as a blob
        size_t writeOffset = m_Storage.size();
        m_Storage.resize(writeOffset + sizeof(executeLambda));
        memcpy(m_Storage.data() + writeOffset, &carrier, sizeof(carrier));

        m_Lambdas.push_back({
            .StateOffset = writeOffset,
            .Lambda = Carrier::LambdaCaller,
        });
    }
};

} // namespace Engine::Core::Rendering
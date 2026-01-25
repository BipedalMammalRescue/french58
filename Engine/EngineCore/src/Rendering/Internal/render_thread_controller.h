#pragma once

#include "EngineCore/Rendering/render_target.h"
#include "EngineCore/Rendering/render_thread_controller.h"
#include <vector>

namespace Engine::Core::Rendering::Internal {

class RenderThreadController : public IRenderThreadController
{
private:
    // NOTE: is this a good place to store the render targets?
    std::vector<RenderTarget> m_RenderTargets;
    size_t m_BuiltInRenderTargetCount;

public:
    RenderThreadController(RenderTarget opaqueTarget, RenderTarget depthTarget);

    ColorAttachmentTarget GetOpaqueTarget() const;
    DepthAttachmentTarget GetOpaqueDepthTarget() const;

    inline void Reset()
    {
        m_RenderTargets.resize(m_BuiltInRenderTargetCount);
    }
};

} // namespace Engine::Core::Rendering::Internal
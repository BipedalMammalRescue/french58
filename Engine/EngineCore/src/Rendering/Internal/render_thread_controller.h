#pragma once

#include "EngineCore/Rendering/render_target.h"
#include "EngineCore/Rendering/render_thread_controller.h"

namespace Engine::Core::Rendering::Internal {

class RenderThreadController : public IRenderThreadController
{
private:
    // NOTE: is this a good place to store the render targets?
    size_t m_BuiltInRenderTargetCount;

public:
    ColorAttachmentTarget GetOpaqueTarget() const;
    DepthAttachmentTarget GetOpaqueDepthTarget() const;
};

} // namespace Engine::Core::Rendering::Internal
#include "render_thread_controller.h"
#include "EngineCore/Rendering/render_target.h"

Engine::Core::Rendering::ColorAttachmentTarget Engine::Core::Rendering::Internal::
    RenderThreadController::GetOpaqueTarget() const
{
    ColorAttachmentTarget result;
    result.m_Id = 0;
    return result;
}

Engine::Core::Rendering::DepthAttachmentTarget Engine::Core::Rendering::Internal::
    RenderThreadController::GetOpaqueDepthTarget() const
{
    DepthAttachmentTarget result;
    result.m_Id = 1;
    return result;
}

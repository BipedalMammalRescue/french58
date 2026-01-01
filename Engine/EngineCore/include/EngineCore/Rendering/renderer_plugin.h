#pragma once

namespace Engine::Core::Rendering {

// notes:
// the renderer plugin is ultimately responsible for populating the command buffer;
// becuase the command buffer is hidden, we need some controller object to process the behavior
// this interface should completely decouple client implementation from how the command buffers are
// structured

struct RendererPlugin
{
};

} // namespace Engine::Core::Rendering
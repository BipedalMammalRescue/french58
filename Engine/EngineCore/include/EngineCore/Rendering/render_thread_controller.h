#pragma once

#include <cstddef>
#include <cstdint>
namespace Engine::Core::Rendering {

class IRenderThreadController
{
public:
    virtual uint32_t CreateUnifomrBuffer(size_t size) = 0;
    virtual void SetUniformData(uint32_t id, void *data, size_t size) = 0;
};

} // namespace Engine::Core::Rendering
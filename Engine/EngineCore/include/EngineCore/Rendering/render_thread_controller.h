#pragma once

#include <cstddef>
#include <cstdint>
namespace Engine::Core::Rendering {

class IRenderStateUpdateWriter
{
public:
    virtual void Write(void *data, size_t length) = 0;
};

class IRenderStateUpdateReader
{
public:
    virtual size_t Read(void *buffer, size_t desiredLength) = 0;
};

class IRenderThreadController
{
};

} // namespace Engine::Core::Rendering
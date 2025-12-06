#pragma once

#include "SDL3/SDL_gpu.h"

namespace Engine::Extension::OrcaRendererModule::Assets {

struct Mesh
{
    size_t IndexCount;
    SDL_GPUBuffer *VertexBuffer;
    SDL_GPUBuffer *IndexBuffer;
};

} // namespace Engine::Extension::OrcaRendererModule::Assets
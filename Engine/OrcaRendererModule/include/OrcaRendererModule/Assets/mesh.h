#pragma once

#include "SDL3/SDL_gpu.h"

namespace Engine::Extension::OrcaRendererModule::Assets {

struct Mesh
{
    SDL_GPUBuffer* VertexBuffer;
    SDL_GPUBuffer* IndexBuffer;
};

}
#pragma once

#include "EngineCore/Rendering/renderer_service.h"

namespace Engine::Core::Runtime {

struct RuntimeServices 
{
    Rendering::RendererService* RendererService;
};

}
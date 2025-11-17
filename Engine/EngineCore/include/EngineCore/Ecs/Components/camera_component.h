#pragma once

#include "EngineCore/Pipeline/component_definition.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineUtils/Memory/memstream_lite.h"

namespace Engine::Core::Ecs::Components {

bool CompileCameraComponent(Pipeline::RawComponent input, std::ostream* output);
Runtime::CallbackResult LoadCameraComponent(size_t count, Utils::Memory::MemStreamLite& stream, Runtime::ServiceTable* services, void* moduleState);

// Supposedly it's helpful to treat the position of a camera as a basic concept owned by the core engine
struct Camera
{
    int Entity;
    bool IsPrimary;
};

}
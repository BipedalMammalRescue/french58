#pragma once

#include "EngineCore/Pipeline/hash_id.h"
namespace Engine::Extension::OrcaRendererModule {

// exact design tbd but we want it to be the mediator of all resource lookups
class ResourceDatabase
{
public:
    void* GetResourceFromRenderer(Core::Pipeline::HashId rendererId, Core::Pipeline::HashId resourceName);
};

}
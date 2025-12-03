#include "OrcaRendererModule/Runtime/renderer_resource.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "SDL3/SDL_stdinc.h"

using namespace Engine::Extension::OrcaRendererModule::Runtime;

int CompareRendererResource(const void* lhs, const void* rhs)
{
    auto lhsCast = (RendererResource*)lhs;
    auto rhsCast = (RendererResource*)rhs;

    if (lhsCast->Name > rhsCast->Name)
        return 1;
    else if (lhsCast->Name < rhsCast->Name)
        return -1;
    return 0;
}

RendererResource* RendererResourceCollection::Find(Engine::Core::Pipeline::HashId name)
{
    RendererResource temp {
        .Name = name
    };

    return (RendererResource*)SDL_bsearch(&temp, Resources, Count, sizeof(RendererResource), CompareRendererResource);
}
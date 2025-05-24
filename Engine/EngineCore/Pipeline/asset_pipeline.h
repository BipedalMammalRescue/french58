#pragma once

#include "Scripting/named_property.h"
#include "Scripting/variant.h"
#include <DependencyInjection/buildtime_services.h>
#include <DependencyInjection/runtime_services.h>

namespace Engine::Core::Pipeline {

struct AssetDefinition
{
    const char *Name;
    const Scripting::NamedProperty *Properties;
    size_t PropertyCount;
};

// compared to components, assets are slightly simpler on the user side, since they are mostly managed by the engine as
// individual bianry blocks
struct AssetPipeline
{
    AssetDefinition Definition;

    void (*Build)(const Scripting::Variant *fieldv, size_t fieldc, DependencyInjection::BuildtimeServies *services,
                  std::ostream &output);

    AssetManagement::LoadedAsset (*Load)(const unsigned char *inputDataV, const size_t inputDataC, const uint64_t id,
                                         DependencyInjection::RuntimeServices *services);

    void (*Dispose)(AssetManagement::LoadedAsset asset, const uint64_t id,
                    DependencyInjection::RuntimeServices *services);
};

// syntactic sugar for creating an asset pipeline from a well-formatted class
template <typename T> AssetPipeline CreateAssetPipeline()
{
    static AssetPipeline cache = {T::GetDefinition(), T::Build, T::Load, T::Dispose};

    return cache;
}

} // namespace Engine::Core::Pipeline
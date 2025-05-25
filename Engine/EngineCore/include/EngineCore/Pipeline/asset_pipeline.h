#pragma once

#include "Scripting/named_property.h"
#include "Scripting/variant.h"
#include "EngineCore/DependencyInjection/buildtime_services.h"
#include "EngineCore/DependencyInjection/runtime_services.h"

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

    // takes in an asset file, outputs the binary data that the engine should store and use for running game
    bool (*Build)(const Scripting::Variant *fieldv, size_t fieldc, DependencyInjection::BuildtimeServies *services,
                  std::ostream &output);

    // used to measure the loaded size of an asset
    size_t (*MaxLoadSize)(const unsigned char *inputDataV, const size_t inputDataC, const uint64_t id,
                          DependencyInjection::RuntimeServices *services);

    // load an asset into the asset manager
    AssetManagement::LoadedAsset (*Load)(const unsigned char *inputDataV, const size_t inputDataC, const uint64_t id,
                                         DependencyInjection::RuntimeServices *services);

    // called before an asset is unloaded from memory
    void (*Dispose)(AssetManagement::LoadedAsset asset, const uint64_t id,
                    DependencyInjection::RuntimeServices *services);
};

// syntactic sugar for creating an asset pipeline from a well-formatted class
template <typename T> AssetPipeline CreateAssetPipeline()
{
    static AssetPipeline cache = {T::GetDefinition(), T::Build, T::MaxLoadSize, T::Load, T::Dispose};
    return cache;
}

} // namespace Engine::Core::Pipeline
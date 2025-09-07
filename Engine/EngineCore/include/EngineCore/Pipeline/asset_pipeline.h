#pragma once

#include "EngineCore/DependencyInjection/buildtime_services.h"
#include "Scripting/named_property.h"
#include "Scripting/variant.h"

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
                 std::ostream *output);
};

} // namespace Engine::Core::Pipeline
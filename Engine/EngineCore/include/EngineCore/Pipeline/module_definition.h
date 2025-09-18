#pragma once

#include "EngineCore/AssetManagement/asset_header.h"
#include "EngineCore/Runtime/runtime_services.h"

#include <array>
#include <istream>
#include <md5.h>

namespace Engine::Core::Pipeline {

struct AssetDefinition 
{
    std::array<unsigned char, 16> Name;

    // Load an asset; provided is the header of the asset and the rest of the input;
    // Engine services are injected for operations like renderer resource registration;
    // Output should be written into the output pointer, buffer size is guaranteed to be consistent with header.
    void (*Load)(AssetManagement::AssetHeader* header, Runtime::RuntimeServices* services, std::istream* input, void* output);
    void (*Unload)(Runtime::RuntimeServices* services, void* output);
};

// includes everything defined in a module
struct ModuleDefinition
{
    // static definitions
    std::array<unsigned char, 16> Name;

    // assets
    const AssetDefinition* Assets;
    size_t AssetsCount;
};

class ModuleBuilderExample
{
public:
    size_t CountAssets() { return 10; }
    void Build(ModuleDefinition& def) {}
};

} // namespace Engine::Core::Pipeline
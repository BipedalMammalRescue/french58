#pragma once

#include <array>
#include <md5.h>

namespace Engine::Core::Pipeline {

struct AssetDefinition 
{
    std::array<unsigned char, 16> Name;
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
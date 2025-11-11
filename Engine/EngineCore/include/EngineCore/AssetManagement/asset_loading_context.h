#pragma once

#include "EngineCore/Pipeline/hash_id.h"

namespace Engine::Core::AssetManagement {

enum class LoadBufferType
{
    TransientBuffer,
    ModuleBuffer
};

struct LoadBuffer
{
    union {
        size_t TransientBuffer; // contextualizer writes the required length here, loader writes an ID before indexing
        void* ModuleBuffer; // this buffer is fully allocated before the contextualizer exits
    } Location;
    LoadBufferType Type;
};

struct AssetLoadingContext
{
    Pipeline::HashIdTuple AssetGroupId;
    Pipeline::HashId AssetId;
    size_t SourceSize;
    LoadBuffer Buffer;
};

}
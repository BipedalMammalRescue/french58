#pragma once

#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/AssetManagement/async_io_event.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/asset_definition.h"
#include "EngineCore/Pipeline/component_definition.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/transient_allocator.h"
#include "EngineUtils/Memory/memstream_lite.h"
#include "SDL3/SDL_asyncio.h"
#include <vector>

namespace Engine::Core::Runtime {

class ModuleManager;
class TaskManager;
class TransientAllocator;
class WorldState;
struct ServiceTable;

class AssetManager
{
private:
    friend class GameLoop;

    // generate them somehow
    std::unordered_map<Pipeline::HashIdTuple, Pipeline::ComponentDefinition> m_Components;
    std::unordered_map<Pipeline::HashIdTuple, Pipeline::AssetDefinition> m_AssetDefinitions;

    // services
    ServiceTable* m_Services;
    ModuleManager* m_ModuleManager;
    TaskManager* m_TaskManager;
    TransientAllocator* m_Allocator;
    WorldState* m_WorldState;
    Logging::Logger m_Logger;

    // the main thread: manages these task queues
    std::vector<AssetManagement::AssetLoadingContext> m_ContexualizeQueue;
    std::vector<AssetManagement::AsyncAssetEvent> m_IndexQueue;
    std::vector<AssetManagement::AsyncEntityEvent> m_EntityLoadingQueue;

    // asynchronous event handling
    CallbackResult PollEvents();
    
    void OnEntityReady(TransientBufferId buffer, Pipeline::HashId id);
    void ProcessComponentGroup(Utils::Memory::MemStreamLite &stream,
                               Pipeline::HashIdTuple componentGroupId,
                               int componentCount);
    void OnAssetReady(AssetManagement::AssetLoadingContext context);

    // actually asynchronous: use the asyncIO API to load data (this implementation assumes loose data; contain all IO code in here so we can swap out asset system backend)
    SDL_AsyncIOQueue* m_AsyncQueue;
    bool LoadAssetFileAsync(AssetManagement::AssetLoadingContext* destination);
    bool LoadEntityFileAsync(Pipeline::HashId id);

  public:
    void QueueEntity(Pipeline::HashId entityId);
};

}
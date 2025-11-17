#pragma once

#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/AssetManagement/async_io_event.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Logging/logger_service.h"
#include "EngineCore/Pipeline/asset_definition.h"
#include "EngineCore/Pipeline/component_definition.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/module_assembly.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/transient_allocator.h"
#include "EngineUtils/Memory/memstream_lite.h"
#include "SDL3/SDL_asyncio.h"
#include "EngineCore/Runtime/index_queue.h"
#include "SDL3/SDL_storage.h"

#include <unordered_map>
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

    // used for utilities
    SDL_Storage* m_StorageFolder;

    // generate them somehow
    std::unordered_map<Pipeline::HashIdTuple, Pipeline::ComponentDefinition> m_Components;
    std::unordered_map<Pipeline::HashIdTuple, Pipeline::AssetDefinition> m_AssetDefinitions;

    // services
    Logging::Logger m_Logger;
    ServiceTable* m_Services;

    // contextualization
    std::vector<AssetManagement::AssetLoadingContext> m_AssetReloadQueue;
    std::vector<AssetManagement::AssetLoadingContext> m_ContextualizeQueue;

    // indexing
    IndexQueue* m_DependencyAgnosticIndexQueue;
    std::unordered_map<Pipeline::HashId, IndexQueue*> m_IndexQueues;

    std::vector<Pipeline::HashId> m_EntityScheduleQueue;
    std::vector<AssetManagement::AsyncEntityEvent> m_EntityLoadingQueue;

    // asynchronous event handling
    CallbackResult ProcessIndexQueue(IndexQueue*& queue);
    CallbackResult PollEvents();
    
    void OnEntityReady(TransientBufferId buffer, Pipeline::HashId id);
    void ProcessComponentGroup(Utils::Memory::MemStreamLite &stream,
                               Pipeline::HashIdTuple componentGroupId,
                               int componentCount);
    void OnAssetReady(AssetManagement::AssetLoadingContext context);

    // actually asynchronous: use the asyncIO API to load data (this implementation assumes loose data; contain all IO code in here so we can swap out asset system backend)
    SDL_AsyncIOQueue* m_AsyncQueue;
    bool LoadAssetFileAsync(AssetManagement::AsyncAssetEvent* destination);
    bool LoadEntityFileAsync(Pipeline::HashId id);

    // Queue an enetity to be loaded at the immediate next possible timing. Assets are loaded based on the implementation of engine (e.g. if eventually asseet bundles/packs are supported they'll go through that path)
    void QueueEntity(Pipeline::HashId entityId);

    size_t GetAssetSize(Pipeline::HashId assetId);

public:
    AssetManager(Engine::Core::Pipeline::ModuleAssembly modules, Logging::LoggerService *loggerService, ServiceTable *services);
    ~AssetManager();

    // Queue an individual asset to be loaded at the immeidate next possible timing. Assets are loaded as loose assets regardless of engine implementation.
    void QueueAsset(Pipeline::HashId moduleId, Pipeline::HashId typeId, Pipeline::HashId assetId);
};

}
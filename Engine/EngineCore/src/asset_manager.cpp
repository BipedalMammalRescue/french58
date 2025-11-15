#include "EngineCore/Runtime/asset_manager.h"
#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/AssetManagement/async_io_event.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/module_assembly.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/module_manager.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Runtime/transient_allocator.h"
#include "EngineUtils/Memory/memstream_lite.h"
#include "EngineCore/Runtime/world_state.h"
#include "EngineUtils/String/hex_strings.h"
#include "SDL3/SDL_asyncio.h"
#include "SDL3/SDL_error.h"
#include <md5.h>

using namespace Engine::Core::Runtime;
using namespace Engine::Utils::Memory;

void AssetManager::QueueEntity(Pipeline::HashId entityId)
{
    char nameBuffer[] = "2D87CCD68F05994578FAFA7AF7750AB4.bse_entity";
    Utils::String::BinaryToHex(sizeof(entityId), entityId.Hash.data(), nameBuffer);
    SDL_AsyncIO *ioObject = SDL_AsyncIOFromFile(nameBuffer, "r");

    if (ioObject == nullptr)
    {
        m_Logger.Error("Entity {} can't be opened for read, details: {}", entityId, SDL_GetError());
        return;
    }

    size_t size = SDL_GetAsyncIOSize(ioObject);
    auto bufferId = m_Services->TransientAllocator->CreateBufferGroup(size, 1);
    void* buffer = m_Services->TransientAllocator->GetBuffer(bufferId);

    if (buffer == nullptr)
    {
        m_Logger.Error("Entity {} can't be loaded because transient buffer can't be allocated.", entityId);
        return;
    }

    m_EntityLoadingQueue.push_back(AssetManagement::AsyncEntityEvent(bufferId, size, entityId));
    
    if (!SDL_ReadAsyncIO(ioObject, buffer, 0, size, m_AsyncQueue, &*(m_EntityLoadingQueue.end() - 1)))
    {
        m_Logger.Error("Entity {} can't be loaded, details: {}", entityId, SDL_GetError());
    }

    m_Logger.Information("Entity {} queued for loading.", entityId);
}

bool AssetManager::LoadAssetFileAsync(AssetManagement::AsyncAssetEvent* destination)
{
    char nameBuffer[] = "2D87CCD68F05994578FAFA7AF7750AB4.bse_asset";
    Utils::String::BinaryToHex(sizeof(destination->GetContext()->AssetId), destination->GetContext()->AssetId.Hash.data(), nameBuffer);
    SDL_AsyncIO *ioObject = SDL_AsyncIOFromFile(nameBuffer, "r");

    switch (destination->GetContext()->Buffer.Type)
    {
    case AssetManagement::LoadBufferType::TransientBuffer:
        {
            void* dest = m_Services->TransientAllocator->GetBuffer(destination->GetContext()->Buffer.Location.TransientBufferId);
            if (dest == nullptr)
            {
                m_Logger.Error("Error loading asset {}: transient allocator rejected buffer.", destination->GetContext()->AssetId);
                return false;
            }

            if (!SDL_ReadAsyncIO(ioObject, dest, 0, destination->GetContext()->SourceSize, m_AsyncQueue, destination))
            {
                m_Logger.Error("Error loading asset {}: {}", destination->GetContext()->AssetId, SDL_GetError());
                return false;
            }

            return true;
        }
    case AssetManagement::LoadBufferType::ModuleBuffer:
        {
            if (!SDL_ReadAsyncIO(ioObject, destination->GetContext()->Buffer.Location.ModuleBuffer, 0, destination->GetContext()->SourceSize, m_AsyncQueue, destination))
            {
                m_Logger.Error("Error loading asset {}: {}", destination->GetContext()->AssetId, SDL_GetError());
                return false;
            }
            return true;
        }
    case AssetManagement::LoadBufferType::Invalid:
        m_Logger.Warning("Requested async asset loading on invalid loading context for asset {}.", destination->GetContext()->AssetId);
        return false;
    }
}


struct TransientBufferReturnHelper
{
    TransientBufferId Id;
    TransientAllocator* Allocator;

    ~TransientBufferReturnHelper()
    {
        Allocator->Return(Id);
    }
};


static bool CheckMagicWord(unsigned int target, MemStreamLite& stream)
{
    unsigned int getWord = stream.Read<unsigned int>();
    return target == getWord;
}


CallbackResult AssetManager::PollEvents()
{
    // receive async IO events
    SDL_AsyncIOOutcome lastResult;
    while (SDL_GetAsyncIOResult(m_AsyncQueue, &lastResult))
    {
        // we'll use null userdata for events not worth tracking
        if (lastResult.userdata == nullptr)
            continue;

        // close file
        SDL_CloseAsyncIO(lastResult.asyncio, false, m_AsyncQueue, nullptr);

        auto resource = static_cast<AssetManagement::AsyncIoEvent*>(lastResult.userdata);
        switch (resource->GetType())
        {
        case AssetManagement::EventType::Entity:
            {
                auto entityEvent = static_cast <AssetManagement::AsyncEntityEvent*>(resource);

                switch (lastResult.result)
                {
                case SDL_ASYNCIO_COMPLETE:
                    {
                        m_Logger.Information("Entity ready for processing: {}", entityEvent->GetId());
                        TransientBufferReturnHelper helper = { entityEvent->GetBuffer(), m_Services->TransientAllocator };

                        MemStreamLite stream { m_Services->TransientAllocator->GetBuffer(helper.Id) };
                        size_t readCount = 0;

                        // read the asset section
                        if (!CheckMagicWord(0xCCBBFFF1, stream))
                        {
                            m_Logger.Error("Entity {} magic word for asset section mismatch (loading skipped).", entityEvent->GetId());
                            break;
                        }

                        // queue asset groups for contextualization
                        int assetGroupCount = stream.Read<int>();
                        for (int assetGroupIndex = 0; assetGroupIndex < assetGroupCount; assetGroupIndex ++)
                        {
                            Pipeline::HashIdTuple assetGroupId = stream.Read<Pipeline::HashIdTuple>();
                            int groupSize = stream.Read<int>();

                            // note here asset groups are always continuous in the contextualization queue
                            m_ContextualizeQueue.reserve(m_ContextualizeQueue.size() + groupSize);
                            for (int assetIndex = 0; assetIndex < groupSize; assetIndex ++)
                            {
                                Pipeline::HashId nextAssetId = stream.Read<Pipeline::HashId>();
                                size_t nextAssetSize = stream.Read<size_t>();
                                m_ContextualizeQueue.push_back(AssetManagement::AssetLoadingContext{
                                    nextAssetSize,
                                    assetGroupId,
                                    nextAssetId,
                                    {
                                        {},
                                        AssetManagement::LoadBufferType::Invalid
                                    }
                                });
                            }
                        }

                        // read the entities
                        if (!CheckMagicWord(0xCCBBFFF2, stream))
                        {
                            m_Logger.Error("Entity {} magic word for entity section mismatch (loading skipped).", entityEvent->GetId());
                            break;
                        }
                        if (!m_Services->WorldState->LoadEntities(stream))
                        {
                            m_Logger.Error("Entity {} rejected by world state.", entityEvent->GetId());
                            break;
                        }

                        // read the components
                        if (!CheckMagicWord(0xCCBBFFF3, stream))
                        {
                            m_Logger.Error("Entity {} magic word for component section mismatch (loading skipped).", entityEvent->GetId());
                            break;
                        }

                        int componentGroupCount = stream.Read<int>();
                        for (int componentGroupIndex = 0; componentGroupIndex < componentGroupCount; componentGroupIndex ++)
                        {
                            Pipeline::HashIdTuple componentGroupId = stream.Read<Pipeline::HashIdTuple>();
                            int componentCount = stream.Read<int>();
                            size_t componentStorageOffset = stream.Read<size_t>();
                            size_t savedAddress = stream.GetPosition();

                            stream.Seek(componentStorageOffset);
                            size_t sectionLength = stream.Read<size_t>();
                            ProcessComponentGroup(stream, componentGroupId, componentCount);
                            stream.Seek(savedAddress);
                        }

                        m_Logger.Information("Loaded entity including {} asset groups, {} component groups.", assetGroupCount, componentGroupCount);
                        break;
                    }
                    break;
                case SDL_ASYNCIO_FAILURE:
                    m_Logger.Warning("Entity {} loading failed, detail: {}.", 
                        entityEvent->GetId(),
                        SDL_GetError()
                    );
                    break;
                case SDL_ASYNCIO_CANCELED:
                    m_Logger.Warning("Entity {} loading canceled by operating system.", 
                        entityEvent->GetId()
                    );
                    break;
                }
            }
            break;
        case AssetManagement::EventType::Asset:
            {
                auto asset = static_cast<AssetManagement::AsyncAssetEvent*>(resource);
                switch (lastResult.result)
                {
                case SDL_ASYNCIO_COMPLETE:
                    asset->MakeAvailable();
                    m_Logger.Information("Asset {} loaded and made ready for indexing.", asset->GetContext()->AssetId);
                    break;
                case SDL_ASYNCIO_FAILURE:
                    asset->MakeBroken();
                    m_Logger.Warning("Asset {} loading failed, detail: {}.", 
                        asset->GetContext()->AssetId, 
                        SDL_GetError()
                    );
                    break;
                case SDL_ASYNCIO_CANCELED:
                    asset->MakeBroken();
                    m_Logger.Warning("Asset loading canceled, asset: {}, module: {}, type: {}.", asset->GetContext()->AssetId, asset->GetContext()->AssetGroupId.First, asset->GetContext()->AssetGroupId.Second);
                    break;
                }
            }
            break;
        }

    }

    // TODO: need to refactor this to batch index as well since we already keep all index requests in the same order they show up in the entity file
    // TODO: need to make the index queue a circular buffer => contextualize queue now is a resizable buffer which is fine
    // try to flush the index queue
    if (!m_IndexQueue.empty())
    {
        size_t availableIndexQueue = 0;

        for (availableIndexQueue = 0; availableIndexQueue < m_IndexQueue.size() && (m_IndexQueue[availableIndexQueue].IsAvailable() || m_IndexQueue[availableIndexQueue].IsBroken()); availableIndexQueue ++)
        {
            // skip broken assets
            if (m_IndexQueue[availableIndexQueue].IsBroken())
                continue;

            // run the index routine
            m_Logger.Information("Indexing asset {} {}.", m_IndexQueue[availableIndexQueue].GetDefinition()->Name.DisplayName, m_IndexQueue[availableIndexQueue].GetContext()->AssetId);
            Runtime::CallbackResult result = m_IndexQueue[availableIndexQueue].GetDefinition()->Index(m_Services, m_IndexQueue[availableIndexQueue].GetModuleState(), m_IndexQueue[availableIndexQueue].GetContext());
            if (result.has_value())
                return result;

            if (m_IndexQueue[availableIndexQueue].GetContext()->Buffer.Type == AssetManagement::LoadBufferType::TransientBuffer)
            {
                m_Services->TransientAllocator->Return(m_IndexQueue[availableIndexQueue].GetContext()->Buffer.Location.TransientBufferId);
            }
        }

        // remove the first n elements from the index queue that's been completed
        if (availableIndexQueue > 0)
        {
            m_IndexQueue.erase(m_IndexQueue.begin(), m_IndexQueue.begin() + availableIndexQueue);
        }
    }

    // process contextualized assets
    if (!m_ContextualizeQueue.empty())
    {
        // TODO: IMPORTANT THIS IS AN ERROR, REALLOCATING INDEX QUEUE CAUSES MEMORY LEAKS WE NEED TO FIX THE UDNERLYING LENGTH
        // make room in index queue to reduce small allocations
        m_IndexQueue.reserve(m_IndexQueue.size() + m_ContextualizeQueue.size());

        // transient buffers are batch processed
        size_t transientBufferBudget = 0;
        int transientBufferCount = 0;
        size_t previousIndexQueueOffset = m_IndexQueue.size();

        size_t cursor = 0;
        Pipeline::HashIdTuple currentGroupId = { md5::compute(""), md5::compute("") };
        while (cursor < m_ContextualizeQueue.size())
        {
            currentGroupId = m_ContextualizeQueue[cursor].AssetGroupId;

            size_t contextGroupSize = 0;
            // asset sections are written in groups
            while (cursor + contextGroupSize < m_ContextualizeQueue.size() && m_ContextualizeQueue[cursor + contextGroupSize].AssetGroupId == currentGroupId)
            {
                contextGroupSize ++;
            }

            auto targetAssetType = m_AssetDefinitions.find(currentGroupId);
            if (targetAssetType == m_AssetDefinitions.end())
            {
                m_Logger.Warning("Definition not found for asset type {}:{}, assets will be skipped", 
                    currentGroupId.First, 
                    currentGroupId.Second);
            }
            else 
            {
                auto targetModuleState = m_Services->ModuleManager->FindModuleMutable(currentGroupId.First);
                if (targetModuleState == nullptr)
                {
                    m_Logger.Warning("Module state not found for asset type {}:{}, loading skipped.", 
                        currentGroupId.First, 
                        currentGroupId.Second);
                }
                else 
                {
                    m_Logger.Information("Contextualizing asset group {} ({}:{})", 
                        targetAssetType->second.Name.DisplayName,
                        currentGroupId.First, 
                        currentGroupId.Second
                    );

                    // batch contextualize
                    auto result = targetAssetType->second.Contextualize(m_Services, targetModuleState, &m_ContextualizeQueue[cursor], contextGroupSize);
                    if (result.has_value())
                        return result;

                    // schedule them for IO and indexing
                    for (size_t i = cursor; i < cursor + contextGroupSize; i++)
                    {
                        if (m_ContextualizeQueue[i].Buffer.Type == AssetManagement::LoadBufferType::Invalid)
                        {
                            m_Logger.Information("Loading context rejected, asset: {}, module: {}, type: {}; loading skipped.", 
                                m_ContextualizeQueue[i].AssetId, 
                                m_ContextualizeQueue[i].AssetGroupId.First, 
                                m_ContextualizeQueue[i].AssetGroupId.Second);
                            continue;
                        }

                        // push the new event into index queue; this will be the persistent location until the task is finished (we need to pass opaque pointers around)
                        m_IndexQueue.push_back(AssetManagement::AsyncAssetEvent(m_ContextualizeQueue[i], &targetAssetType->second, targetModuleState));
                        AssetManagement::AsyncAssetEvent* persistentCopy = &*(m_IndexQueue.end() - 1);

                        switch (persistentCopy->GetContext()->Buffer.Type)
                        {
                        case AssetManagement::LoadBufferType::TransientBuffer:
                            // we batch transient requests together to reduce memory alllocation frequency
                            transientBufferBudget += m_ContextualizeQueue[i].Buffer.Location.TransientBufferSize;
                            transientBufferCount++;
                            break;
                        case AssetManagement::LoadBufferType::ModuleBuffer:
                            LoadAssetFileAsync(persistentCopy);
                            break;
                        default:
                            break;
                        }
                    }
                }
            }

            cursor += contextGroupSize;
            currentGroupId.First = md5::compute("");
        }

        // allocate transient buffer and schedule the transient buffer tasks
        if (transientBufferBudget > 0)
        {
            TransientBufferId bufferId = m_Services->TransientAllocator->CreateBufferGroup(transientBufferBudget, transientBufferCount);

            int offset = 0;
            for (size_t i = previousIndexQueueOffset; i < m_IndexQueue.size(); i++)
            {
                if (m_IndexQueue[i].GetContext()->Buffer.Type != AssetManagement::LoadBufferType::TransientBuffer)
                    continue;

                // grant a portion of the transient buffer
                bufferId.Child = offset;
                offset += m_IndexQueue[i].GetContext()->Buffer.Location.TransientBufferSize;
                m_IndexQueue[i].GetContext()->Buffer.Location.TransientBufferId = bufferId;

                // schedule IO
                LoadAssetFileAsync(&m_IndexQueue[i]);
            }
        }
    }
    m_ContextualizeQueue.clear();

    return CallbackSuccess();
}

void Engine::Core::Runtime::AssetManager::ProcessComponentGroup(Utils::Memory::MemStreamLite &stream, Pipeline::HashIdTuple componentGroupId, int componentCount)
{
    auto targetComponent = m_Components.find(componentGroupId);
    if (targetComponent == m_Components.end())
    {
        m_Logger.Warning("Definition not found for component: {}:{}, loading skipped.", componentGroupId.First, componentGroupId.Second);
        return;
    }

    auto targetModuleState = m_Services->ModuleManager->FindModuleMutable(componentGroupId.First);
    if (targetModuleState == nullptr)
    {
        m_Logger.Warning("Module state not found for component: {}:{}, loading skipped.", componentGroupId.First, componentGroupId.Second);
        return;
    }

    m_Logger.Information("Loading component group: {}.", targetComponent->second.Name.DisplayName);
    targetComponent->second.Load(componentCount, stream, m_Services, targetModuleState);
}
Engine::Core::Runtime::AssetManager::AssetManager(Engine::Core::Pipeline::ModuleAssembly modules, Logging::LoggerService *loggerService, ServiceTable *services)
    : m_Logger(loggerService->CreateLogger("AssetManager")),
      m_Services(services) 
{
    m_AsyncQueue = SDL_CreateAsyncIOQueue();

    // build component table
    for (size_t i = 0; i < modules.ModuleCount; i++)
    {
        Pipeline::ModuleDefinition module = modules.Modules[i];
        for (size_t j = 0; j < module.ComponentCount; j ++)
        {
            Pipeline::ComponentDefinition component = module.Components[j];
            Pipeline::HashIdTuple tuple { module.Name.Hash, component.Name.Hash };
            m_Components[tuple] = component;
        }
    }

    // build asset table
    for (size_t i = 0; i < modules.ModuleCount; i++)
    {
        Pipeline::ModuleDefinition module = modules.Modules[i];
        for (size_t j = 0; j < module.AssetsCount; j ++)
        {
            Pipeline::AssetDefinition asset = module.Assets[j];
            Pipeline::HashIdTuple tuple { module.Name.Hash, asset.Name.Hash };
            m_AssetDefinitions[tuple] = asset;
        }
    }

    m_IndexQueue.reserve(1024);
}

#include "EngineCore/Runtime/asset_manager.h"
#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/AssetManagement/async_io_event.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/module_manager.h"
#include "EngineCore/Runtime/transient_allocator.h"
#include "EngineUtils/Memory/memstream_lite.h"
#include "EngineCore/Runtime/world_state.h"
#include "EngineUtils/String/hex_strings.h"
#include "SDL3/SDL_asyncio.h"
#include "SDL3/SDL_error.h"

using namespace Engine::Core::Runtime;
using namespace Engine::Utils::Memory;

// currently this can only work for the first entity
void AssetManager::QueueEntity(Pipeline::HashId entityId)
{
    char nameBuffer[] = "2D87CCD68F05994578FAFA7AF7750AB4.bse_asset";
}

bool AssetManager::LoadAssetFileAsync(AssetManagement::AssetLoadingContext* destination)
{
    char nameBuffer[] = "2D87CCD68F05994578FAFA7AF7750AB4.bse_asset";
    Utils::String::BinaryToHex(sizeof(destination->AssetId), destination->AssetId.Hash.data(), nameBuffer);
    SDL_AsyncIO *ioObject = SDL_AsyncIOFromFile(nameBuffer, "r");

    switch (destination->Buffer.Type)
    {
    case AssetManagement::LoadBufferType::TransientBuffer:
        {
            void* dest = m_Allocator->GetBuffer(destination->Buffer.Location.TransientBufferId);
            if (dest == nullptr)
            {
                m_Logger.Error("Error loading asset {}: transient allocator rejected buffer.", destination->AssetId);
                return false;
            }

            if (!SDL_ReadAsyncIO(ioObject, dest, 0, destination->SourceSize, m_AsyncQueue, destination))
            {
                m_Logger.Error("Error loading asset {}: {}", destination->AssetId, SDL_GetError());
                return false;
            }

            return true;
        }
    case AssetManagement::LoadBufferType::ModuleBuffer:
        {
            if (!SDL_ReadAsyncIO(ioObject, destination->Buffer.Location.ModuleBuffer, 0, destination->SourceSize, m_AsyncQueue, destination))
            {
                m_Logger.Error("Error loading asset {}: {}", destination->AssetId, SDL_GetError());
                return false;
            }
            return true;
        }
    case AssetManagement::LoadBufferType::Invalid:
        m_Logger.Warning("Requested async asset loading on invalid loading context for asset {}.", destination->AssetId);
        return false;
    }
}

CallbackResult AssetManager::PollEvents()
{
    // receive async IO events
    SDL_AsyncIOOutcome lastResult;
    while (SDL_GetAsyncIOResult(m_AsyncQueue, &lastResult))
    {
        auto resource = static_cast<AssetManagement::AsyncIoEvent*>(lastResult.userdata);

        switch (resource->GetType())
        {
        case AssetManagement::EventType::Entity:
            // TODO: implement entity loading
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
                    m_Logger.Warning("Asset loading failed, asset: {}, module: {}, type: {}, detail: {}.", 
                        asset->GetContext()->AssetId, 
                        asset->GetContext()->AssetGroupId.First, 
                        asset->GetContext()->AssetGroupId.Second,
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

    // try to flush the index queue
    if (!m_IndexQueue.empty())
    {
        size_t availableIndexQueue = 0;

        while (m_IndexQueue[availableIndexQueue].IsAvailable() || m_IndexQueue[availableIndexQueue].IsBroken())
        {
            // skip broken assets
            if (m_IndexQueue[availableIndexQueue].IsBroken())
                continue;

            // run the index routine
            auto targetAssetType = m_AssetDefinitions.find(m_IndexQueue[availableIndexQueue].GetContext()->AssetGroupId);
            if (targetAssetType == m_AssetDefinitions.end())
            {
                m_Logger.Warning("Module state not found for asset: {module}:{type}, assets will be skipped", 
                    m_IndexQueue[availableIndexQueue].GetContext()->AssetGroupId.First, 
                    m_IndexQueue[availableIndexQueue].GetContext()->AssetGroupId.Second);
                continue;
            }

            auto targetModuleState = m_ModuleManager->FindModuleMutable(m_IndexQueue[availableIndexQueue].GetContext()->AssetGroupId.First);
            if (targetModuleState == nullptr)
            {
                m_Logger.Warning("Module state not found for asset: {}:{}, loading skipped.", 
                    m_IndexQueue[availableIndexQueue].GetContext()->AssetGroupId.First, 
                    m_IndexQueue[availableIndexQueue].GetContext()->AssetGroupId.Second);
                continue;
            }

            Runtime::CallbackResult result = targetAssetType->second.Index(m_Services, targetModuleState, m_IndexQueue[availableIndexQueue].GetContext());
            if (result.has_value())
                return result;

            availableIndexQueue ++;
        }

        // remove the first n elements from the index queue that's been completed
        if (availableIndexQueue > 0)
        {
            m_IndexQueue.erase(m_IndexQueue.begin(), m_IndexQueue.begin() + availableIndexQueue);
        }
    }

    // process contextualized assets
    if (!m_ContexualizeQueue.empty())
    {
        // make room in index queue to reduce small allocations
        m_IndexQueue.reserve(m_IndexQueue.size() + m_ContexualizeQueue.size());

        // transient buffers are batch processed
        size_t transientBufferBudget = 0;
        int transientBufferCount = 0;
        size_t previousIndexQueueOffset = m_IndexQueue.size();

        size_t cursor = 0;
        size_t contextGroupSize = 0;
        Pipeline::HashIdTuple groupId = m_ContexualizeQueue[0].AssetGroupId;
        while (cursor + contextGroupSize < m_ContexualizeQueue.size())
        {
            // asset sections are written in groups
            while (cursor + contextGroupSize < m_ContexualizeQueue.size() && m_ContexualizeQueue[cursor + contextGroupSize].AssetGroupId == groupId)
            {
                contextGroupSize ++;
            }

            m_Logger.Information("Contextualizing asset: {id}, ({module}:{type})", 
                m_ContexualizeQueue[cursor + contextGroupSize].AssetGroupId.First, 
                m_ContexualizeQueue[cursor + contextGroupSize].AssetGroupId.Second
            );

            if (contextGroupSize > 0)
            {
                auto targetAssetType = m_AssetDefinitions.find(m_ContexualizeQueue[cursor + contextGroupSize].AssetGroupId);
                if (targetAssetType == m_AssetDefinitions.end())
                {
                    m_Logger.Warning("Module state not found for asset: {module}:{type}, assets will be skipped", 
                        m_ContexualizeQueue[cursor + contextGroupSize].AssetGroupId.First, 
                        m_ContexualizeQueue[cursor + contextGroupSize].AssetGroupId.Second);
                    continue;
                }

                auto targetModuleState = m_ModuleManager->FindModuleMutable(m_ContexualizeQueue[cursor + contextGroupSize].AssetGroupId.First);
                if (targetModuleState == nullptr)
                {
                    m_Logger.Warning("Module state not found for asset: {}:{}, loading skipped.", 
                        m_ContexualizeQueue[cursor + contextGroupSize].AssetGroupId.First, 
                        m_ContexualizeQueue[cursor + contextGroupSize].AssetGroupId.Second);
                    continue;
                }

                // batch contextualize
                auto result = targetAssetType->second.Contextualize(m_Services, targetModuleState, &m_ContexualizeQueue[cursor], contextGroupSize);
                if (result.has_value())
                    return result;

                // schedule them for IO and indexing
                for (size_t i = cursor; i < contextGroupSize; i++)
                {
                    if (m_ContexualizeQueue[i].Buffer.Type == AssetManagement::LoadBufferType::Invalid)
                    {
                        m_Logger.Information("Loading context rejected, asset: {}, module: {}, type: {}; loading skipped.", 
                            m_ContexualizeQueue[i].AssetId, 
                            m_ContexualizeQueue[i].AssetGroupId.First, 
                            m_ContexualizeQueue[i].AssetGroupId.Second);
                        continue;
                    }

                    // push the new event into index queue; this will be the persistent location until the task is finished (we need to pass opaque pointers around)
                    m_IndexQueue.push_back(AssetManagement::AsyncAssetEvent(m_ContexualizeQueue[i]));
                    AssetManagement::AsyncAssetEvent* persistentCopy = &*(m_IndexQueue.end() - 1);

                    switch (persistentCopy->GetContext()->Buffer.Type)
                    {
                    case AssetManagement::LoadBufferType::TransientBuffer:
                        // we batch transient requests together to reduce memory alllocation frequency
                        transientBufferBudget += m_ContexualizeQueue[i].Buffer.Location.TransientBufferSize;
                        transientBufferCount++;
                        break;
                    case AssetManagement::LoadBufferType::ModuleBuffer:
                        {
                            m_IndexQueue.push_back(AssetManagement::AsyncAssetEvent(m_ContexualizeQueue[i]));
                            AssetManagement::AsyncAssetEvent* persistentCopy = &*(m_IndexQueue.end() - 1);

                            // schedule IO
                            // TODO: rn don't handle it; the game will be in a *correct* but not desirable state and it's fine; during debugging we can force reload an asset
                            LoadAssetFileAsync(persistentCopy->GetContext());
                        }
                        break;
                    default:
                        break;
                    }
                }
            }

            cursor += contextGroupSize;
            contextGroupSize = 0;
        }

        // allocate transient buffer and schedule the transient buffer tasks
        if (transientBufferBudget > 0)
        {
            TransientBufferId bufferId = m_Allocator->CreateBufferGroup(transientBufferBudget, transientBufferCount);

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
                LoadAssetFileAsync(m_IndexQueue[i].GetContext());
            }
        }

        m_ContexualizeQueue.clear();
    }

    return CallbackSuccess();
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

void AssetManager::OnEntityReady(TransientBufferId bufferId, Pipeline::HashId entityId)
{
    m_Logger.Information("Entity ready for processing: {}", entityId);
    TransientBufferReturnHelper helper = { bufferId, m_Allocator };

    MemStreamLite stream { m_Allocator->GetBuffer(bufferId) };
    size_t readCount = 0;

    // read the asset section
    if (!CheckMagicWord(0xCCBBFFF1, stream))
    {
        m_Logger.Error("Entity magic word for asset section mismatch (loading skipped); entity id: {}.", entityId);
        return;
    }

    // queue assets for contexualization
    int assetGroupCount = stream.Read<int>();
    for (int assetGroupIndex = 0; assetGroupIndex < assetGroupCount; assetGroupIndex ++)
    {
        Pipeline::HashIdTuple assetGroupId = stream.Read<Pipeline::HashIdTuple>();
        int groupSize = stream.Read<int>();

        m_ContexualizeQueue.reserve(m_ContexualizeQueue.size() + groupSize);
        for (int assetIndex = 0; assetIndex < groupSize; assetIndex ++)
        {
            Pipeline::HashId nextAssetId = stream.Read<Pipeline::HashId>();
            size_t nextAssetSize = stream.Read<size_t>();

            AssetManagement::AssetLoadingContext context;
            context.AssetGroupId = assetGroupId;
            context.AssetId = nextAssetId;
            context.SourceSize = nextAssetSize;
            context.Buffer.Type = AssetManagement::LoadBufferType::Invalid;

            // TODO
            // // reserve a position in the asset table; it'll be filled in later
            // m_LoadedAsset[nextAssetId] = context;
        }
    }

    // read the entities
    if (!CheckMagicWord(0xCCBBFFF2, stream))
    {
        m_Logger.Error("Entity magic word for entity section mismatch (loading skipped); entity id: {}.", entityId);
        return;
    }
    if (!!m_WorldState->LoadEntities(stream))
    {
        m_Logger.Error("Entity rejected by world state; entity id: {}.");
        return;
    }

    // read the components
    if (!CheckMagicWord(0xCCBBFFF3, stream))
    {
        m_Logger.Error("Entity magic word for component section mismatch (loading skipped); entity id: {}.", entityId);
        return;
    }

    int componentGroupCount = stream.Read<int>();
    for (int componentGroupIndex = 0; componentGroupIndex < componentGroupCount; componentGroupIndex ++)
    {
        Pipeline::HashIdTuple componentGroupId = stream.Read<Pipeline::HashIdTuple>();
        int componentCount = stream.Read<int>();
        size_t componentStorageOffset = stream.Read<size_t>();
        size_t savedAddress = stream.GetPosition();

        stream.Seek(componentStorageOffset);
        ProcessComponentGroup(stream, componentGroupId, componentCount);
        stream.Seek(savedAddress);
    }

    m_Logger.Information("Contextualized {} asset groups, loaded {} component groups.", assetGroupCount, componentGroupCount);
    return;
}

void Engine::Core::Runtime::AssetManager::ProcessComponentGroup(Utils::Memory::MemStreamLite &stream, Pipeline::HashIdTuple componentGroupId, int componentCount)
{
    auto targetComponent = m_Components.find(componentGroupId);
    if (targetComponent == m_Components.end())
    {
        m_Logger.Warning("Definition not found for component: {}:{}, loading skipped.", componentGroupId.First, componentGroupId.Second);
        return;
    }

    auto targetModuleState = m_ModuleManager->FindModuleMutable(componentGroupId.First);
    if (targetModuleState == nullptr)
    {
        m_Logger.Warning("Module state not found for component: {}:{}, loading skipped.", componentGroupId.First, componentGroupId.Second);
        return;
    }

    targetComponent->second.Load(componentCount, stream, m_Services, targetModuleState);
}
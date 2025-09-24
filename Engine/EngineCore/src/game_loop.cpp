#include "EngineCore/Runtime/game_loop.h"

#include "EngineCore/Pipeline/asset_definition.h"
#include "EngineCore/Pipeline/asset_enumerable.h"
#include "EngineCore/Pipeline/engine_callback.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/module_definition.h"
#include "EngineCore/Runtime/graphics_layer.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Runtime/world_state.h"
#include "EngineCore/Runtime/module_manager.h"
#include "EngineUtils/String/hex_strings.h"

#include <SDL3/SDL_events.h>
#include <fstream>

using namespace Engine::Core::Runtime;

enum class FileIoResult
{
    Success,
    NotOpened,
    Corrupted,
    AssetGroupNotFound,
    ModuleNotFound,
    ComponentGroupNotFound
};

GameLoop::GameLoop(Pipeline::ModuleAssembly modules) : 
    m_ConfigurationProvider(),
    m_Modules(modules)
{
    // build component table
    for (size_t i = 0; i < modules.ModuleCount; i++)
    {
        Pipeline::ModuleDefinition module = modules.Modules[i];
        for (size_t j = 0; j < module.ComponentCount; j ++)
        {
            Pipeline::ComponentDefinition component = module.Components[j];
            Pipeline::HashIdTuple tuple { module.Name, component.Name };
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
            Pipeline::HashIdTuple tuple { module.Name, asset.Name };
            m_Assets[tuple] = asset;
        }
    }
}

struct InstancedCallback
{
    void (*Callback)(ServiceTable* services, void* moduleState);
    void* InstanceState;
};

int GameLoop::Run() 
{
    GraphicsLayer graphicsLayer(&m_ConfigurationProvider);
    WorldState worldState(&m_ConfigurationProvider);
    ModuleManager moduleManager;

    // initialize services
    if (!graphicsLayer.InitializeSDL())
        return 1;

    ServiceTable services {
        &graphicsLayer,
        &worldState,
        &moduleManager
    };

    std::vector<InstancedCallback> renderCallbacks;
    moduleManager.m_LoadedModules.reserve(m_Modules.ModuleCount);
    
    // load modules
    for (size_t i = 0; i < m_Modules.ModuleCount; i++)
    {
        Pipeline::ModuleDefinition moduleDef = m_Modules.Modules[i];
        void* newState = moduleDef.Initialize(&services);
        moduleManager.LoadModule(moduleDef.Name, newState);

        // set up callback table
        if (moduleDef.CallbackCount > 0)
        {
            for (size_t j = 0; j < moduleDef.CallbackCount; j++) 
            {
                Pipeline::EngineCallback callback = moduleDef.Callbacks[j];
                switch (callback.Stage)
                {
                    case Pipeline::EngineCallbackStage::Preupdate:
                    case Pipeline::EngineCallbackStage::ScriptUpdate:
                    case Pipeline::EngineCallbackStage::ModuleUpdate:
                        break;
                    case Pipeline::EngineCallbackStage::Render:
                        renderCallbacks.push_back({ callback.Callback, newState });
                        break;
                }
            }
        }
    }

    // TODO: load the initial scene

    bool quit = false;
	SDL_Event e;
	while (!quit)
	{
		// Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
            //User requests quit
			quit = e.type == SDL_EVENT_QUIT;
		}

		// begin update loop
		graphicsLayer.BeginFrame();

        // pre update

        // script update

        // module update

        // render pass
        for (auto& callback : renderCallbacks)
        {
            callback.Callback(&services, callback.InstanceState);
        }
        
        // last step in the update loop
		graphicsLayer.EndFrame();
	}

    return 0;
}

class StreamAssetEnumerator : public Engine::Core::Pipeline::IAssetEnumerator
{
private:
    int m_Cursor = -1;
    int m_Count = 0;
    std::istream* m_Source = nullptr;
    std::ifstream m_AssetFile;
    Engine::Core::Pipeline::HashId m_AssetId = { {0} };
    char m_NameBuffer[43] = "CD0ED230BD87479C61DB68677CAA9506.bse_asset";

public:
    StreamAssetEnumerator(std::istream* source)
    {
        source->read((char*)&m_Count, sizeof(int));
        m_Source = source;
    }

    size_t Count() override 
    {
        return m_Count;
    };

    bool MoveNext() override 
    {
        if (m_Cursor + 1 >= m_Count)
            return false;

        m_Cursor ++;

        // close the original file
        if (m_AssetFile.is_open())
        {
            m_AssetFile.close();
        }

        // open the new file
        m_Source->read((char*)m_AssetId.Hash.data(), 16);
        Engine::Utils::String::BinaryToHex(16, m_AssetId.Hash.data(), m_NameBuffer);
        m_AssetFile.open(m_NameBuffer, std::ios::binary);

        return m_AssetFile.is_open();
    }

    Engine::Core::Pipeline::RawAsset GetCurrent() override 
    {
        return { &m_AssetFile, m_AssetId };
    }
};

static bool CheckMagicWord(unsigned int target, std::istream* input)
{
    unsigned int getWord = 0;
    input->read((char*)&getWord, sizeof(unsigned int));
    return target == getWord;
}

int GameLoop::LoadEntity(const char* filePath, ServiceTable services)
{
    std::ifstream entityFile;
    entityFile.open(filePath);

    if (!entityFile.is_open())
        return static_cast<int>(FileIoResult::NotOpened);

    // read the asset section
    if (!CheckMagicWord(0xCCBBFFF1, &entityFile))
        return static_cast<int>(FileIoResult::Corrupted);
    int assetGroupCount = 0;
    entityFile.read((char*)&assetGroupCount, sizeof(int));
    for (int assetGroupIndex = 0; assetGroupIndex < assetGroupCount; assetGroupIndex ++)
    {
        Pipeline::HashIdTuple assetGroupId;
        entityFile.read((char*)&assetGroupId, 32);

        auto targetAssetType = m_Assets.find(assetGroupId);
        if (targetAssetType == m_Assets.end())
            return static_cast<int>(FileIoResult::AssetGroupNotFound);

        auto targetModuleState = services.ModuleManager->m_LoadedModules.find(assetGroupId.First);
        if (targetModuleState == services.ModuleManager->m_LoadedModules.end())
            return static_cast<int>(FileIoResult::ModuleNotFound);

        StreamAssetEnumerator enumerator(&entityFile);
        targetAssetType->second.Load(&enumerator, &services, targetModuleState->second);
    }

    // read the entities
    if (!CheckMagicWord(0xCCBBFFF2, &entityFile) || !services.WorldState->LoadEntities(&entityFile))
        return static_cast<int>(FileIoResult::Corrupted);

    // read the components
    if (!CheckMagicWord(0xCCBBFFF3, &entityFile))
        return static_cast<int>(FileIoResult::Corrupted);
    int componentGroupCount = 0;
    entityFile.read((char*)&componentGroupCount, sizeof(int));
    for (int componentGroupIndex = 0; componentGroupIndex < componentGroupCount; componentGroupIndex ++)
    {
        Pipeline::HashIdTuple componentGroupId;
        entityFile.read((char*)&componentGroupId, 32);
        
        auto targetComponent = m_Components.find(componentGroupId);
        if (targetComponent == m_Components.end())
            return (int)FileIoResult::ComponentGroupNotFound;

        auto targetModuleState = services.ModuleManager->m_LoadedModules.find(componentGroupId.First);
        if (targetModuleState == services.ModuleManager->m_LoadedModules.end())
            return static_cast<int>(FileIoResult::ModuleNotFound);

        int componentCount = 0;
        entityFile.read((char*)&componentCount, sizeof(int));
        targetComponent->second.Load(componentCount, &entityFile, &services, targetModuleState->second);
    }

    return (int)FileIoResult::Success;
}
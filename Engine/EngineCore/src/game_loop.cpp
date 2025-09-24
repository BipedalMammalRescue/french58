#include "EngineCore/Runtime/game_loop.h"

#include "EngineCore/Pipeline/engine_callback.h"
#include "EngineCore/Pipeline/module_definition.h"
#include "EngineCore/Runtime/service_table.h"
#include "SDL3/SDL_events.h"

using namespace Engine::Core::Runtime;

GameLoop::GameLoop(Pipeline::ModuleAssembly modules) : 
    m_ConfigurationProvider(), 
    m_GraphicsLayer(&m_ConfigurationProvider), 
    m_WorldState(&m_ConfigurationProvider),
    m_ModuleManager(),
    m_Modules(modules) {}

struct InstancedCallback
{
    void (*Callback)(ServiceTable* services, void* moduleState);
    void* InstanceState;
};

int GameLoop::Run() 
{
    // initialize services
    if (!m_GraphicsLayer.InitializeSDL())
        return 1;

    ServiceTable services {
        &m_GraphicsLayer,
        &m_WorldState,
        &m_ModuleManager
    };

    std::vector<InstancedCallback> renderCallbacks;
    m_ModuleManager.m_LoadedModules.reserve(m_Modules.ModuleCount);
    
    // load modules
    for (size_t i = 0; i < m_Modules.ModuleCount; i++)
    {
        Pipeline::ModuleDefinition moduleDef = m_Modules.Modules[i];
        void* newState = moduleDef.Initialize(&services);
        m_ModuleManager.LoadModule(moduleDef.Name, newState);

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
		m_GraphicsLayer.BeginFrame();

        // pre update

        // script update

        // module update

        // render pass
        for (auto& callback : renderCallbacks)
        {
            callback.Callback(&services, callback.InstanceState);
        }
        
        // last step in the update loop
		m_GraphicsLayer.EndFrame();
	}

    return 0;
}
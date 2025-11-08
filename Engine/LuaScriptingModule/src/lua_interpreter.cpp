#include "EngineCore/Runtime/crash_dump.h"
#include "LuaScriptingModule/lua_executor.h"
#include "EngineCore/Configuration/configuration_provider.h"
#include "EngineCore/Pipeline/module_assembly.h"
#include "EngineCore/Runtime/game_loop.h"
#include <iostream>


int main()
{
    Engine::Core::Runtime::GameLoop gameloop(Engine::Core::Pipeline::ListModules(), Engine::Core::Configuration::ConfigurationProvider());
    gameloop.DiagnsoticMode([](Engine::Core::Runtime::IGameLoopController* controller)
    {
        controller->Initialize();
        controller->LoadModules();

        std::cout << "*** LUA RUNTIME START ***" << std::endl;
        Engine::Extension::LuaScriptingModule::LuaExecutor executor(controller->GetServices());

        Engine::Core::Runtime::CallbackResult result = executor.ExecuteFile("test.lua");
        if (result.has_value())
        {
            std::cout << "Lua Error: " << std::endl;
            std::cout << result->ErrorDetail << std::endl;
        }

        std::cout << "*** LUA RUNTIME END ***" << std::endl;
    });
}
#include "EngineCore/Pipeline/module_assembly.h"
#include "EngineCore/Runtime/game_loop.h"

int main()
{
    Engine::Core::Runtime::GameLoop gameloop(Engine::Core::Pipeline::ListModules());
    return gameloop.Run();
}

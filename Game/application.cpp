#include "EngineCore/Pipeline/module_assembly.h"
#include "EngineCore/Runtime/game_loop.h"
#include <md5.h>

int main()
{
    Engine::Core::Runtime::GameLoop gameloop(Engine::Core::Pipeline::ListModules());
    return gameloop.Run(md5::compute("Entities/example.se_entity"));
}

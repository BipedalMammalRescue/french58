#include <Hosting/engine_host.h>
#include <renderer_module.h>

using namespace Engine::Core;

int main()
{
    Hosting::EngineHost engineHost;
    engineHost.AddModule<Extension::RendererModule::RendererModule>();
    // engineHost.AddSinkModule<DummySinkModule>();
    return engineHost.Run();
}

#pragma once

namespace Engine::Extension::OrcaRendererModule::Components {

class SceneRenderer 
{

public:
    void* GetGeometry() const;
    void* GetSurfaceData() const;
};

}
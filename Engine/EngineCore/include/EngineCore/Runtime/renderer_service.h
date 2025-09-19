#pragma once

#include "EngineCore/Logging/logger_service.h"
#include "EngineCore/Rendering/renderer_data.h"
#include <SDL3/SDL_gpu.h>
#include <glm/fwd.hpp>

#include <string>
#include <unordered_map>

struct SDL_GPUBuffer;
struct SDL_GPUShader;

namespace Engine::Core::Runtime {

class PlatformAccess;

// Renderer service owns another set of SDL api outside of PlatformAccess as the ultimate endpoint for any rendering
// behavior.
// TODO: add render texture
class RendererService
{
  private:
    Logging::LoggerService *m_Logger = Logging::GetLogger();
    Runtime::PlatformAccess *m_Platform = nullptr;

  public:
    RendererService(Runtime::PlatformAccess *platform) : m_Platform(platform)
    {
    }

    ~RendererService() = default;

  public:
    bool CompileShader(const unsigned char *code, size_t codeLength, Rendering::ShaderType type,
                       unsigned int numSamplers, unsigned int numUniformBuffers, unsigned int numStorageBuffers,
                       unsigned int numSotrageTextures, Rendering::RendererShader &outID);
    bool DeleteShader(Rendering::RendererShader &shader);

    bool CreateMaterial(const Rendering::RendererShader &vertexShader, const Rendering::RendererShader &fragmentShader,
                        Rendering::VertexAttribute *layouts, unsigned int layoutCount, Rendering::RendererMaterial &outID);
    bool DeleteMaterial(Rendering::RendererMaterial &material);

    bool RegisterMesh(const Rendering::VertexCollection &vertices, const Rendering::IndexCollection &indices,
                      Rendering::RendererMesh &outID);
    bool DeleteMesh(Rendering::RendererMesh &inID);

    // rendering regular object would only require a MVP, and every object we assume has an MVP
    // maybe signature-annotate the puroose of this function?
    // TODO: how should this api work with uniforms?
    // WE CAN technically get some temp solutions?
    bool QueueRender(Rendering::RendererMesh *mesh, Rendering::RendererMaterial *material, const glm::mat4 &mvp);
};

} // namespace Engine::Core::Runtime
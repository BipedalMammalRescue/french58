#include "OrcaRendererModule/Assets/shader.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/graphics_layer.h"
#include "OrcaRendererModule/orca_renderer_module.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_gpu.h"

using namespace Engine::Extension::OrcaRendererModule::Assets;
using namespace Engine::Extension::OrcaRendererModule;

// TODO: at some point we need to make this configurable
const SDL_GPUVertexAttribute VertexAttributes[] = {
    {0, 0, SDL_GPUVertexElementFormat::SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0},
    {1, 0, SDL_GPUVertexElementFormat::SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, sizeof(float) * 3},
    {2, 0, SDL_GPUVertexElementFormat::SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, sizeof(float) * 6}};

const SDL_GPUVertexBufferDescription vertexBufferDescription{
    0, sizeof(float) * 8, SDL_GPUVertexInputRate::SDL_GPU_VERTEXINPUTRATE_VERTEX, 0};

const SDL_GPUVertexInputState vertexInputState{&vertexBufferDescription, 1, VertexAttributes, 3};

// Represents the shader file on disk, containing two files and their length prepended; the entry
// point must be called "main".
// TODO: how do we store the pass information? Should the engine support slicing the input file into
// regions or something?
struct SerializedShader
{
    struct ShaderStage
    {
        size_t CodeSize;
        uint32_t UniformCount;
        uint32_t StorageBufferCount;
        uint32_t TextureCount;
        uint32_t SamplerCount;

        const unsigned char *GetCode() const
        {
            return (const unsigned char *)(this + 1);
        }
    };

    const ShaderStage *GetVertexShader() const
    {
        return (ShaderStage *)(this + 1);
    }

    const ShaderStage *GetFragmentShader() const
    {
        return (ShaderStage *)(GetVertexShader()->GetCode() + GetVertexShader()->CodeSize);
    }
};

Engine::Core::Runtime::CallbackResult Assets::ContextualizeShader(
    Engine::Core::Runtime::ServiceTable *services, void *moduleState,
    Engine::Core::AssetManagement::AssetLoadingContext *outContext, size_t contextCount)
{
    ModuleState *state = static_cast<ModuleState *>(moduleState);

    for (size_t i = 0; i < contextCount; i++)
    {
        // TODO: use bi-linear search
        // check if the shader already exists
        if (!outContext[i].ReplaceExisting && state->FindShader(outContext[i].AssetId) != nullptr)
        {
            state->GetLogger()->Information("Shader {} is already loaded.", outContext[i].AssetId);
            outContext[i].Buffer.Type = Engine::Core::AssetManagement::LoadBufferType::Invalid;
        }
        else
        {
            outContext[i].Buffer.Type =
                Engine::Core::AssetManagement::LoadBufferType::TransientBuffer;
            outContext[i].Buffer.Location.TransientBufferSize = outContext[i].SourceSize;
        }
    }

    return Engine::Core::Runtime::CallbackSuccess();
}

static SDL_GPUShader *CreateShader(SDL_GPUShaderStage stage,
                                   const SerializedShader::ShaderStage *shader,
                                   SDL_GPUDevice *device)
{
    SDL_GPUShaderCreateInfo vertexShaderCreateInfo{.code_size = shader->CodeSize,
                                                   .code = shader->GetCode(),
                                                   .entrypoint = "main",
                                                   .format = SDL_GPU_SHADERFORMAT_SPIRV,
                                                   .stage = SDL_GPU_SHADERSTAGE_VERTEX,
                                                   .num_samplers = shader->SamplerCount,
                                                   .num_storage_textures = shader->TextureCount,
                                                   .num_storage_buffers =
                                                       shader->StorageBufferCount,
                                                   .num_uniform_buffers = shader->UniformCount};
    return SDL_CreateGPUShader(device, &vertexShaderCreateInfo);
}

Engine::Core::Runtime::CallbackResult Assets::IndexShader(
    Engine::Core::Runtime::ServiceTable *services, void *moduleState,
    Engine::Core::AssetManagement::AssetLoadingContext *inContext)
{
    Engine::Extension::OrcaRendererModule::ModuleState *state =
        static_cast<Engine::Extension::OrcaRendererModule::ModuleState *>(moduleState);

    SerializedShader *loadedShader = (SerializedShader *)services->TransientAllocator->GetBuffer(
        inContext->Buffer.Location.TransientBufferId);
    if (loadedShader == nullptr)
    {
        state->GetLogger()->Error("Transient buffer invalid.");
        return Core::Runtime::CallbackSuccess();
    }

    // get the vertex shader
    const SerializedShader::ShaderStage *vertexShader = loadedShader->GetVertexShader();
    SDL_GPUShader *gpuVertShader = CreateShader(SDL_GPU_SHADERSTAGE_VERTEX, vertexShader,
                                                services->GraphicsLayer->GetDevice());

    if (gpuVertShader == nullptr)
    {
        state->GetLogger()->Error("Failed to create vertex shader, detail: {}", SDL_GetError());
        return Core::Runtime::CallbackSuccess();
    }

    // get the fragment shader
    const SerializedShader::ShaderStage *fragmentShader = loadedShader->GetFragmentShader();
    SDL_GPUShader *gpuFragShader = CreateShader(SDL_GPU_SHADERSTAGE_FRAGMENT, fragmentShader,
                                                services->GraphicsLayer->GetDevice());

    if (gpuFragShader == nullptr)
    {
        state->GetLogger()->Error("Failed to create fragment shader, detail: {}", SDL_GetError());
        SDL_ReleaseGPUShader(services->GraphicsLayer->GetDevice(), gpuVertShader);
        return Core::Runtime::CallbackSuccess();
    }

    // build PSO
    SDL_GPUColorTargetDescription colorTarget{SDL_GetGPUSwapchainTextureFormat(
        services->GraphicsLayer->GetDevice(), services->GraphicsLayer->GetWindow())};

    SDL_GPUGraphicsPipelineCreateInfo psoCreateInfo{
        .vertex_shader = gpuVertShader,
        .fragment_shader = gpuFragShader,
        .vertex_input_state = vertexInputState,
        .primitive_type = SDL_GPUPrimitiveType::SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = {.cull_mode = SDL_GPU_CULLMODE_BACK},
        .depth_stencil_state = {.compare_op = SDL_GPU_COMPAREOP_LESS,
                                .enable_depth_test = true,
                                .enable_depth_write = true},
        .target_info = {.color_target_descriptions = &colorTarget,
                        .num_color_targets = 1,
                        .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
                        .has_depth_stencil_target = true}};

    SDL_GPUGraphicsPipeline *gpuPipeline =
        SDL_CreateGPUGraphicsPipeline(services->GraphicsLayer->GetDevice(), &psoCreateInfo);
    if (gpuPipeline == nullptr)
    {
        state->GetLogger()->Error("Failed to create gpu graphics pipeline, detail:, {}",
                                  inContext->AssetId, SDL_GetError());
    }

    return Engine::Core::Runtime::CallbackSuccess();
}
#include "OrcaRendererModule/Assets/shader.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/graphics_layer.h"
#include "EngineCore/Runtime/heap_allocator.h"
#include "OrcaRendererModule/orca_renderer_module.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_stdinc.h"

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
// point must be called "main". Layout: header | bindings | vertex stage | fragment stage
struct SerializedShaderEffect
{
    Engine::Core::Pipeline::HashId RenderGraph;
    uint32_t RenderPassId;

    uint32_t ResourceBindingCount;
    const ShaderResourceBinding *GetResourceBindings() const
    {
        return (const ShaderResourceBinding *)(this + 1);
    }

    struct ShaderStage
    {
        uint32_t CodeSize;
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
        // skip binding section
        return (ShaderStage *)(GetResourceBindings() + ResourceBindingCount);
    }

    const ShaderStage *GetFragmentShader() const
    {
        // skip past vertex shader
        return (ShaderStage *)(GetVertexShader()->GetCode() + GetVertexShader()->CodeSize);
    }

    const SerializedShaderEffect *GetNext() const
    {
        // skip past fragment shader
        return (SerializedShaderEffect *)(GetFragmentShader()->GetCode() +
                                          GetFragmentShader()->CodeSize);
    }
};

struct SerializedShader
{
    uint32_t EffectCount;
    const SerializedShaderEffect *GetFirstEffect() const
    {
        return (SerializedShaderEffect *)(this + 1);
    }
};

Engine::Core::Runtime::CallbackResult Assets::ContextualizeShader(
    Engine::Core::Runtime::ServiceTable *services, void *moduleState,
    Engine::Core::AssetManagement::AssetLoadingContext *outContext, size_t contextCount)
{
    ModuleState *state = static_cast<ModuleState *>(moduleState);

    for (size_t i = 0; i < contextCount; i++)
    {
        // TODO: make the asset manager always sort the incoming loading contexts
        // check if the shader already exists
        if (!outContext[i].ReplaceExisting && state->FindShader(outContext[i].AssetId) != nullptr)
        {
            state->Logger.Information("Shader {} is already loaded.", outContext[i].AssetId);
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
                                   const SerializedShaderEffect::ShaderStage *shader,
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

    // create the shader object on the heap
    size_t totalSize = sizeof(Shader) + loadedShader->EffectCount * sizeof(ShaderEffect);
    Shader *newShader = static_cast<Shader *>(services->HeapAllocator->Allocate(totalSize));
    newShader->EffectCount = loadedShader->EffectCount;

    // deserialize all shader effects
    const SerializedShaderEffect *effect = loadedShader->GetFirstEffect();
    for (uint32_t effectIndex = 0; effectIndex < loadedShader->EffectCount; effectIndex++)
    {
        if (effect == nullptr)
        {
            state->Logger.Error("Transient buffer invalid.");
            return Core::Runtime::CallbackSuccess();
        }

        // get the vertex shader
        const SerializedShaderEffect::ShaderStage *vertexShader = effect->GetVertexShader();
        SDL_GPUShader *gpuVertShader = CreateShader(SDL_GPU_SHADERSTAGE_VERTEX, vertexShader,
                                                    services->GraphicsLayer->GetDevice());

        if (gpuVertShader == nullptr)
        {
            state->Logger.Error("Failed to create vertex shader, detail: {}", SDL_GetError());
            services->HeapAllocator->Deallocate(newShader);
            return Core::Runtime::CallbackSuccess();
        }

        // get the fragment shader
        const SerializedShaderEffect::ShaderStage *fragmentShader = effect->GetFragmentShader();
        SDL_GPUShader *gpuFragShader = CreateShader(SDL_GPU_SHADERSTAGE_FRAGMENT, fragmentShader,
                                                    services->GraphicsLayer->GetDevice());

        if (gpuFragShader == nullptr)
        {
            state->Logger.Error("Failed to create fragment shader, detail: {}", SDL_GetError());
            SDL_ReleaseGPUShader(services->GraphicsLayer->GetDevice(), gpuVertShader);
            services->HeapAllocator->Deallocate(newShader);
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
        SDL_ReleaseGPUShader(services->GraphicsLayer->GetDevice(), gpuVertShader);
        SDL_ReleaseGPUShader(services->GraphicsLayer->GetDevice(), gpuFragShader);

        if (gpuPipeline == nullptr)
        {
            state->Logger.Error("Failed to create gpu graphics pipeline, detail:, {}",
                                inContext->AssetId, SDL_GetError());
            services->HeapAllocator->Deallocate(newShader);
            return Core::Runtime::CallbackSuccess();
        }

        // create/find a reference object
        size_t graphReference = state->RenderGraphs.CreateReference(effect->RenderGraph);

        // load up the new shader
        newShader->GetShaderEffects()[effectIndex] = {.RenderGraphReference = graphReference,
                                                      .RenderPassId = effect->RenderPassId,
                                                      .Pipeline = gpuPipeline};

        uint32_t effectiveResourceBindingCount =
            SDL_min(SDL_arraysize(newShader->GetShaderEffects()[effectIndex].Resources),
                    effect->ResourceBindingCount);

        for (uint32_t bindingIndex = 0; bindingIndex < effectiveResourceBindingCount;
             bindingIndex++)
        {
            newShader->GetShaderEffects()[effectIndex].Resources[bindingIndex] =
                effect->GetResourceBindings()[bindingIndex];
        }

        newShader->GetShaderEffects()[effectIndex].ResourceCount = effectiveResourceBindingCount;

        // sort all resources
        SDL_qsort(newShader->GetShaderEffects()[effectIndex].Resources,
                  effectiveResourceBindingCount, sizeof(ShaderResourceBinding),
                  [](const void *a, const void *b) -> int {
                      const ShaderResourceBinding *lhs = (ShaderResourceBinding *)a;
                      const ShaderResourceBinding *rhs = (ShaderResourceBinding *)b;
                      if (lhs->Source.Name > rhs->Source.Name)
                          return 1;
                      else if (lhs->Source.Name < rhs->Source.Name)
                          return -1;
                      else
                          return 0;
                  });

        // increment the reference
        effect = effect->GetNext();
    }

    // add the new shader to collection, free the old one if needed
    Shader *oldCopy = state->Shaders.UpdateReference(inContext->AssetId, newShader);
    if (oldCopy != nullptr)
    {
        state->Renderer.FreeShader(oldCopy);
        state->Logger.Information("Freed previous revision of shader {}.", inContext->AssetId);
    }
    state->Logger.Information("Loaded shader {} with {} effects.", inContext->AssetId,
                              loadedShader->EffectCount);

    return Engine::Core::Runtime::CallbackSuccess();
}
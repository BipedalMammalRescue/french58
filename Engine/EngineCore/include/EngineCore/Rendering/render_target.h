#pragma once

#include "EngineCore/Rendering/gpu_resource.h"
#include <vulkan/vulkan_core.h>
namespace Engine::Core::Rendering {

enum class ColorFormat
{
    UseSwapchain
};

enum class DepthPrecision
{
    D32
};

struct ColorTargetSetting
{
    bool ScaleToSwapchain;

    union {
        struct
        {
            float WidthScale;
            float HeightScale;
        } Relative;
        struct
        {
            uint32_t Width;
            uint32_t Height;
        } Absolute;
    } Dimensions;

    ColorFormat ColorFormat;
};

struct DepthBufferSetting
{
    bool ScaleToSwapchain;

    union {
        struct
        {
            float WidthScale;
            float HeightScale;
        } Relative;
        struct
        {
            uint32_t Width;
            uint32_t Height;
        } Absolute;
    } Dimensions;

    DepthPrecision Precision;
};

enum class SamplerType
{
    None,
    Linear,
    Point
};

struct RenderTargetSetting
{
    union {
        ColorTargetSetting ColorTarget;
        DepthBufferSetting DepthBuffer;
    } Image;
    SamplerType Sampler;
};

// NOTE: not supporting depth stencil yet
enum class RenderTargetUsage
{
    ColorTarget,
    DepthBuffer,
};

struct RenderTarget
{
    RenderTargetSetting Setting;
    GpuImage Image;
    VkImageView View;
};

} // namespace Engine::Core::Rendering
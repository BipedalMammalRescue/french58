// See https://aka.ms/new-console-template for more information
using System.Text.Json;
using DataModels;
using EntityBuilder.Abstractions;

// use a temporary pipeline

Pipeline monochromePhongPipeline = new()
{
    VertexShader = new()
    {
        Path = "foobar",
        Uniforms = [
            new InjectedShaderUniform()
            {
                Set = 1,
                Binding = 0,
                Identifier = "ModelTransform"
            },
            new InjectedShaderUniform()
            {
                Set = 1,
                Binding = 1,
                Identifier = "ProjectionTransform"
            },
            new InjectedShaderUniform()
            {
                Set = 1,
                Binding = 1,
                Identifier = "ViewTransform"
            }
        ]
    },
    FragmentShader = new()
    {
        Path = "foobar",
        StorageBuffers = [
            new InjectedShaderStorageBuffer()
            {
                Set = 2,
                Binding = 0,
                Identifier = "DirectionalLightBuffer"
            }
        ],
        Uniforms = [
            new ConfigurableShaderUniform()
            {
                Set = 3,
                Binding = 0,
                Name = "kSpecular",
                Default = new Variant() { Float = 0.0f }
            },
            new ConfigurableShaderUniform()
            {
                Set = 3,
                Binding = 0,
                Name = "kDiffuse",
                Default = new Variant() { Float = 0.0f }
            },
            new ConfigurableShaderUniform()
            {
                Set = 3,
                Binding = 0,
                Name = "kAmbient",
                Default = new Variant() { Float = 0.0f }
            },
            new ConfigurableShaderUniform()
            {
                Set = 3,
                Binding = 0,
                Name = "kShininess",
                Default = new Variant() { Float = 1.0f }
            },
            new ConfigurableShaderUniform()
            {
                Set = 3,
                Binding = 0,
                Name = "ambientColor",
                Default = new Variant() { Vec3 = [1, 0, 0] }
            },
        ]
    }
};

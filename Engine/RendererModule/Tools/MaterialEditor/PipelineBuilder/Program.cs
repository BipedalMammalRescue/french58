// See https://aka.ms/new-console-template for more information
using CommunityToolkit.HighPerformance;
using DataModels;
using EntityBuilder.Abstractions;

// use a temporary pipeline

Pipeline monochromePhongPipeline = new()
{
    VertexShader = new()
    {
        Uniforms = [
            new DynamicInjectedShaderUniform()
            {
                Binding = 0,
                Identifier = DynamicUniformIdentifier.ModelTransform
            },
            new DynamicInjectedShaderUniform()
            {
                Binding = 1,
                Identifier = DynamicUniformIdentifier.ViewTransform
            },
            new DynamicInjectedShaderUniform()
            {
                Binding = 2,
                Identifier = DynamicUniformIdentifier.ProjectionTransform
            }
        ]
    },
    FragmentShader = new()
    {
        StorageBuffers = [
            new StaticInjectedShaderStorageBuffer()
            {
                Binding = 0,
                Identifier = StaticStorageBufferIdentifier.DirectionalLightBuffer
            }
        ],
        Uniforms = [
            new ConfigurableShaderUniform()
            {
                Binding = 1,
                Name = "kSpecular",
                Default = new Variant() { Float = 0.0f }
            },
            new ConfigurableShaderUniform()
            {
                Binding = 2,
                Name = "kDiffuse",
                Default = new Variant() { Float = 0.0f }
            },
            new ConfigurableShaderUniform()
            {
                Binding = 3,
                Name = "kAmbient",
                Default = new Variant() { Float = 0.0f }
            },
            new ConfigurableShaderUniform()
            {
                Binding = 4,
                Name = "kShininess",
                Default = new Variant() { Float = 1.0f }
            },
            new ConfigurableShaderUniform()
            {
                Binding = 5,
                Name = "ambientColor",
                Default = new Variant() { Vec3 = [1, 0, 0] }
            },
        ]
    }
};

Stream outputStream = Console.OpenStandardOutput();

// static vertex uniform
StaticInjectedShaderUniform[] staticInjectedVertexShaderUniforms = [.. monochromePhongPipeline.VertexShader.Uniforms.OfType<StaticInjectedShaderUniform>()];
outputStream.Write((ulong)staticInjectedVertexShaderUniforms.Length);
foreach (StaticInjectedShaderUniform uniform in staticInjectedVertexShaderUniforms)
{
    outputStream.Write(uniform.Binding);
    outputStream.Write((byte)uniform.Identifier);
}

// static fragment uniform
StaticInjectedShaderUniform[] staticInjectedFragmentShaderUniforms = [.. monochromePhongPipeline.FragmentShader.Uniforms.OfType<StaticInjectedShaderUniform>()];
outputStream.Write((ulong)staticInjectedFragmentShaderUniforms.Length);
foreach (StaticInjectedShaderUniform uniform in staticInjectedFragmentShaderUniforms)
{
    outputStream.Write(uniform.Binding);
    outputStream.Write((byte)uniform.Identifier);
}

// Dynamic vertex uniform
DynamicInjectedShaderUniform[] dynamicInjectedVertexShaderUniforms = [.. monochromePhongPipeline.VertexShader.Uniforms.OfType<DynamicInjectedShaderUniform>()];
outputStream.Write((ulong)dynamicInjectedVertexShaderUniforms.Length);
foreach (DynamicInjectedShaderUniform uniform in dynamicInjectedVertexShaderUniforms)
{
    outputStream.Write(uniform.Binding);
    outputStream.Write((byte)uniform.Identifier);
}

// Dynamic fragment uniform
DynamicInjectedShaderUniform[] dynamicInjectedFragmentShaderUniforms = [.. monochromePhongPipeline.FragmentShader.Uniforms.OfType<DynamicInjectedShaderUniform>()];
outputStream.Write((ulong)dynamicInjectedFragmentShaderUniforms.Length);
foreach (DynamicInjectedShaderUniform uniform in dynamicInjectedFragmentShaderUniforms)
{
    outputStream.Write(uniform.Binding);
    outputStream.Write((byte)uniform.Identifier);
}

// static vertex storage buffers
StaticInjectedShaderStorageBuffer[] staticInjectedVertexShaderStorageBuffers = [.. monochromePhongPipeline.VertexShader.StorageBuffers.OfType<StaticInjectedShaderStorageBuffer>()];
outputStream.Write((ulong)staticInjectedVertexShaderStorageBuffers.Length);
foreach (StaticInjectedShaderStorageBuffer buffer in staticInjectedVertexShaderStorageBuffers)
{
    outputStream.Write(buffer.Binding);
    outputStream.Write((byte)buffer.Identifier);
}

// static Fragment storage buffers
StaticInjectedShaderStorageBuffer[] staticInjectedFragmentShaderStorageBuffers = [.. monochromePhongPipeline.FragmentShader.StorageBuffers.OfType<StaticInjectedShaderStorageBuffer>()];
outputStream.Write((ulong)staticInjectedFragmentShaderStorageBuffers.Length);
foreach (StaticInjectedShaderStorageBuffer buffer in staticInjectedFragmentShaderStorageBuffers)
{
    outputStream.Write(buffer.Binding);
    outputStream.Write((byte)buffer.Identifier);
}

// Dynamic vertex storage buffers
DynamicInjectedShaderStorageBuffer[] dynamicInjectedVertexShaderStorageBuffers = [.. monochromePhongPipeline.VertexShader.StorageBuffers.OfType<DynamicInjectedShaderStorageBuffer>()];
outputStream.Write((ulong)dynamicInjectedVertexShaderStorageBuffers.Length);
foreach (DynamicInjectedShaderStorageBuffer buffer in dynamicInjectedVertexShaderStorageBuffers)
{
    outputStream.Write(buffer.Binding);
    outputStream.Write((byte)buffer.Identifier);
}

// Dynamic Fragment storage buffers
DynamicInjectedShaderStorageBuffer[] dynamicInjectedFragmentShaderStorageBuffers = [.. monochromePhongPipeline.FragmentShader.StorageBuffers.OfType<DynamicInjectedShaderStorageBuffer>()];
outputStream.Write((ulong)dynamicInjectedFragmentShaderStorageBuffers.Length);
foreach (DynamicInjectedShaderStorageBuffer buffer in dynamicInjectedFragmentShaderStorageBuffers)
{
    outputStream.Write(buffer.Binding);
    outputStream.Write((byte)buffer.Identifier);
}

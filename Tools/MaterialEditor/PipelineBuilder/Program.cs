using System.Text.Json;
using CommunityToolkit.HighPerformance;
using DataModels;

internal class Program
{
    private static void Main(string[] args)
    {
        if (args.Length != 1)
        {
            Console.Error.WriteLine("Incorrect command line argument format!");
            Environment.Exit(1);
            return;
        }

        PipelinePrototype pipeline;

        string sourcePath = args[0];
        try
        {
            using FileStream fs = File.OpenRead(sourcePath);
            pipeline = JsonSerializer.Deserialize<PipelinePrototype>(fs) ?? throw new Exception("Null input!");
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine("Failed to read pipeline, error:");
            Console.Error.WriteLine(ex);
            Environment.Exit(2);
            return;
        }

        using Stream outputStream = Console.OpenStandardOutput();

        // TODO: need to prepend the shaders in asset file

        // static vertex uniform
        StaticInjectedShaderUniform[] staticInjectedVertexShaderUniforms = [.. pipeline.VertexShader.Uniforms.OfType<StaticInjectedShaderUniform>()];
        outputStream.Write((ulong)staticInjectedVertexShaderUniforms.Length);
        foreach (StaticInjectedShaderUniform uniform in staticInjectedVertexShaderUniforms)
        {
            outputStream.Write(uniform.Binding);
            outputStream.Write((byte)uniform.Identifier);
        }

        // static fragment uniform
        StaticInjectedShaderUniform[] staticInjectedFragmentShaderUniforms = [.. pipeline.FragmentShader.Uniforms.OfType<StaticInjectedShaderUniform>()];
        outputStream.Write((ulong)staticInjectedFragmentShaderUniforms.Length);
        foreach (StaticInjectedShaderUniform uniform in staticInjectedFragmentShaderUniforms)
        {
            outputStream.Write(uniform.Binding);
            outputStream.Write((byte)uniform.Identifier);
        }

        // Dynamic vertex uniform
        DynamicInjectedShaderUniform[] dynamicInjectedVertexShaderUniforms = [.. pipeline.VertexShader.Uniforms.OfType<DynamicInjectedShaderUniform>()];
        outputStream.Write((ulong)dynamicInjectedVertexShaderUniforms.Length);
        foreach (DynamicInjectedShaderUniform uniform in dynamicInjectedVertexShaderUniforms)
        {
            outputStream.Write(uniform.Binding);
            outputStream.Write((byte)uniform.Identifier);
        }

        // Dynamic fragment uniform
        DynamicInjectedShaderUniform[] dynamicInjectedFragmentShaderUniforms = [.. pipeline.FragmentShader.Uniforms.OfType<DynamicInjectedShaderUniform>()];
        outputStream.Write((ulong)dynamicInjectedFragmentShaderUniforms.Length);
        foreach (DynamicInjectedShaderUniform uniform in dynamicInjectedFragmentShaderUniforms)
        {
            outputStream.Write(uniform.Binding);
            outputStream.Write((byte)uniform.Identifier);
        }

        // static vertex storage buffers
        StaticInjectedShaderStorageBuffer[] staticInjectedVertexShaderStorageBuffers = [.. pipeline.VertexShader.StorageBuffers.OfType<StaticInjectedShaderStorageBuffer>()];
        outputStream.Write((ulong)staticInjectedVertexShaderStorageBuffers.Length);
        foreach (StaticInjectedShaderStorageBuffer buffer in staticInjectedVertexShaderStorageBuffers)
        {
            outputStream.Write(buffer.Binding);
            outputStream.Write((byte)buffer.Identifier);
        }

        // static Fragment storage buffers
        StaticInjectedShaderStorageBuffer[] staticInjectedFragmentShaderStorageBuffers = [.. pipeline.FragmentShader.StorageBuffers.OfType<StaticInjectedShaderStorageBuffer>()];
        outputStream.Write((ulong)staticInjectedFragmentShaderStorageBuffers.Length);
        foreach (StaticInjectedShaderStorageBuffer buffer in staticInjectedFragmentShaderStorageBuffers)
        {
            outputStream.Write(buffer.Binding);
            outputStream.Write((byte)buffer.Identifier);
        }

        // Dynamic vertex storage buffers
        DynamicInjectedShaderStorageBuffer[] dynamicInjectedVertexShaderStorageBuffers = [.. pipeline.VertexShader.StorageBuffers.OfType<DynamicInjectedShaderStorageBuffer>()];
        outputStream.Write((ulong)dynamicInjectedVertexShaderStorageBuffers.Length);
        foreach (DynamicInjectedShaderStorageBuffer buffer in dynamicInjectedVertexShaderStorageBuffers)
        {
            outputStream.Write(buffer.Binding);
            outputStream.Write((byte)buffer.Identifier);
        }

        // Dynamic Fragment storage buffers
        DynamicInjectedShaderStorageBuffer[] dynamicInjectedFragmentShaderStorageBuffers = [.. pipeline.FragmentShader.StorageBuffers.OfType<DynamicInjectedShaderStorageBuffer>()];
        outputStream.Write((ulong)dynamicInjectedFragmentShaderStorageBuffers.Length);
        foreach (DynamicInjectedShaderStorageBuffer buffer in dynamicInjectedFragmentShaderStorageBuffers)
        {
            outputStream.Write(buffer.Binding);
            outputStream.Write((byte)buffer.Identifier);
        }
    }
}
using System.Text.Json;
using CommunityToolkit.HighPerformance;
using DataModels;
using EntityBuilder.Abstractions;

internal class Program
{
    private static void Main(string[] args)
    {
        if (args.Length != 2)
        {
            Console.Error.WriteLine("Incorrect command line argument format!");
            Environment.Exit(1);
            return;
        }

        PipelinePrototype pipeline;
        try
        {
            using FileStream fs = File.OpenRead(args[0]);
            pipeline = JsonSerializer.Deserialize<PipelinePrototype>(fs) ?? throw new Exception("Null input pipeline!");
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine("Failed to read pipeline, error:");
            Console.Error.WriteLine(ex);
            Environment.Exit(2);
            return;
        }

        Material material;
        try
        {
            using FileStream fs = File.OpenRead(args[1]);
            material = JsonSerializer.Deserialize<Material>(fs) ?? throw new Exception("Null input material!");
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine("Failed to read material, error:");
            Console.Error.WriteLine(ex);
            Environment.Exit(3);
            return;
        }

        using Stream outputStream = Console.OpenStandardOutput();
        var writeBuffer = new byte[64];

        // vertex uniforms
        ConfigurableShaderUniform[] vertexUniforms = [.. pipeline.VertexShader.Uniforms.OfType<ConfigurableShaderUniform>()];
        outputStream.Write((long)vertexUniforms.Length);
        foreach (ConfigurableShaderUniform configuredUniform in pipeline.VertexShader.Uniforms.OfType<ConfigurableShaderUniform>())
        {
            outputStream.Write(configuredUniform.Binding);

            // clear write buffer
            for (int i = 0; i < 64; i++)
            {
                writeBuffer[i] = 0;
            }

            // write data
            Variant actualData = material.ConfiguredUniforms.TryGetValue(configuredUniform.Name, out Variant? foundData) && foundData.Type == configuredUniform.Default.Type ? foundData : configuredUniform.Default;
            actualData.Write(writeBuffer);

            outputStream.Write((ulong)actualData.Type);
            outputStream.Write(writeBuffer);
        }

        // fragment uniforms
        ConfigurableShaderUniform[] fragmentUniforms = [.. pipeline.FragmentShader.Uniforms.OfType<ConfigurableShaderUniform>()];
        outputStream.Write((long)fragmentUniforms.Length);
        foreach (ConfigurableShaderUniform configuredUniform in pipeline.FragmentShader.Uniforms.OfType<ConfigurableShaderUniform>())
        {
            outputStream.Write(configuredUniform.Binding);

            // clear write buffer
            for (int i = 0; i < 64; i++)
            {
                writeBuffer[i] = 0;
            }

            // write data
            Variant actualData = (material.ConfiguredUniforms.TryGetValue(configuredUniform.Name, out Variant? foundData) && foundData.Type == configuredUniform.Default.Type) ? foundData : configuredUniform.Default;
            actualData.Write(writeBuffer);

            outputStream.Write((ulong)actualData.Type);
            outputStream.Write(writeBuffer);
        }
    }
}
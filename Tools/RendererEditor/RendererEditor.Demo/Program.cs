// See https://aka.ms/new-console-template for more information
using System.Text.Json;
using RendererEditor.Abstractions;

internal class Program
{
    private static readonly JsonSerializerOptions s_jsonOptions = new()
    {
        WriteIndented = true,
        IndentSize = 4
    };

    private static void Main(string[] args)
    {
        Console.WriteLine("Hello, World!");

        RenderPass pass = new ScenePass()
        {
            Name = "forward_lights",
            InputResources = [],
            ColorTargets = [
                "swapchain_texture"
            ],
            DepthStencilTarget = "main"
        };

        Console.WriteLine(JsonSerializer.Serialize(pass, options: s_jsonOptions));
    }
}
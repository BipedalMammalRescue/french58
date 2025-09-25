// See https://aka.ms/new-console-template for more information
using System.Text.Json;
using EntityBuilder.Abstractions;

Console.WriteLine("Hello, World!");

string code = """
{
    "Module": "RendererModule",
    "Type": "FragmentShader",
    "Action": {
        "$type": "command",
        "Process": "glslc",
        "Arguments": [
            "--fshader-stage=fragment",
            "#(Source)"
        ]
    }
}
""";

string content = "%PATH%";
Console.WriteLine(Environment.ExpandEnvironmentVariables(content));

Recipe? recipe = JsonSerializer.Deserialize<Recipe>(code);
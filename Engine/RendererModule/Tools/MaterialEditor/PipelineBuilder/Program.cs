// See https://aka.ms/new-console-template for more information
using DataModels;

Console.WriteLine("Hello, World!");

// use a temporary pipeline

Pipeline data = new()
{
    VertexShader = new()
    {
        Path = "foobar"
    },
    FragmentShader = new()
    {
        Path = "foobar"
    }
};
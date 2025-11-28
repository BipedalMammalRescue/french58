using System.Text.Json.Serialization;

namespace RendererEditor.Abstractions;

public enum RenderPassType
{
    Scene,
    Screen
}

[JsonPolymorphic(TypeDiscriminatorPropertyName = "Type")]
[JsonDerivedType(typeof(ScenePass), "scene")]
[JsonDerivedType(typeof(ScreenPass), "screen")]
public abstract class RenderPass
{
    [JsonIgnore]
    public abstract RenderPassType PassType { get; }

    public required Dictionary<string, string> InputResources { get; set; }
    public required string[] ColorTargets { get; set; }
    public required string? DepthStencilTarget { get; set; }
}

/// <summary>
/// Scene pass provides extra resource from the objects in the pass.
/// </summary>
public class ScenePass : RenderPass
{
    [JsonIgnore]
    public override RenderPassType PassType => RenderPassType.Scene;
    
    public required string Name { get; set; }
}

/// <summary>
/// Screen passes brings its own geometry (a massive triangle), and therefore no MVP and such.
/// </summary>
public class ScreenPass : RenderPass
{
    public override RenderPassType PassType => RenderPassType.Screen;
    public required string ShaderEffect { get; set; }
}
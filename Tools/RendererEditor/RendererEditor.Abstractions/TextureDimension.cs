using System.Text.Json.Serialization;

namespace RendererEditor.Abstractions;

[JsonPolymorphic(TypeDiscriminatorPropertyName = "Type")]
[JsonDerivedType(typeof(DependentTextureDimension), "dependent")]
[JsonDerivedType(typeof(AbsoluteTextureDimension), "absolute")]
public interface ITextureDimension;

public class DependentTextureDimension : ITextureDimension
{
    public required float ScaleX { get; set; }
    public required float ScaleY { get; set; }
}

public class AbsoluteTextureDimension : ITextureDimension
{
    public required int X { get; set; }
    public required int Y { get; set; }
}
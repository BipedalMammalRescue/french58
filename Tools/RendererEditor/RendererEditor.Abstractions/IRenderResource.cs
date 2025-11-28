using System.Text.Json.Serialization;

namespace RendererEditor.Abstractions;

[JsonPolymorphic(TypeDiscriminatorPropertyName = "Type")]
[JsonDerivedType(typeof(RenderTarget), "render_target")]
public interface IRenderResource
{
    [JsonIgnore]
    RenderResourceType ResourceType { get; }
}

public class RenderTarget : IRenderResource
{
    [JsonIgnore]
    public RenderResourceType ResourceType => RenderResourceType.Texture;
    
    public required TextureFormat Format { get; set; }
    public required HashSet<TextureUsage> Usages { get; set; }
    public required ITextureDimension Dimension { get; set; }
}
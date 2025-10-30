using EntityBuilder.Abstractions;

namespace DataModels;

public class Material
{
    public required Dictionary<string, Variant> ConfiguredUniforms { get; set; }
}

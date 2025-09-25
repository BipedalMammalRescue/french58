namespace EntityBuilder.Abstractions;

public class ComponentSchema
{
    public required string Module { get; set; }
    public required string Type { get; set; }
    public required Dictionary<string, VariantType> Fields { get; set; }
}

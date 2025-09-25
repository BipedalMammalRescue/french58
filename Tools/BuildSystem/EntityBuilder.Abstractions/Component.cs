namespace EntityBuilder.Abstractions;

public class Component
{
    public required string Name { get; set; }
    public required string Module { get; set; }
    public required string Type { get; set; }

    public required Dictionary<string, Variant> Fields { get; set; }
}

using System.Text.Json.Serialization;

namespace EntityBuilder.Abstractions;

public class Entity
{
    public required string Name { get; set; }
    public required Entity[] Children { get; set; }
    public required Component[] Components { get; set; }

    [JsonIgnore]
    public IEnumerable<Component> AllComponents => Children.SelectMany(entity => entity.AllComponents).Concat(Components);

    [JsonIgnore]
    public IEnumerable<string> AllAssets => AllComponents.SelectMany(c => c.Fields.Where(f => f.Value.Type == VariantType.Path).Select(f => f.Value.Path));
}

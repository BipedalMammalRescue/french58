namespace EntityBuilder.Abstractions;

public class Entity
{
    public required string Name { get; set; }

    private Variant _position = default!;
    public required Variant Position
    {
        get => _position;
        set
        {
            if (value.Type != VariantType.Vec3)
                throw new Exception("Entity position must be a vec3.");
            _position = value;
        }
    }

    public required Entity[] Children { get; set; }
    
    public required Component[] Components { get; set; }
}

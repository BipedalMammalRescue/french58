namespace EntityBuilder.Abstractions;

public class Entity
{
    public required string Name { get; set; }

    public required Entity[] Children { get; set; }
    
    public required Component[] Components { get; set; }
}

using System.Text.Json.Serialization;

namespace DataModels;

[JsonPolymorphic]
[JsonDerivedType(typeof(ConfigurableShaderStorageBuffer), typeDiscriminator: "configurable")]
[JsonDerivedType(typeof(InjectedShaderStorageBuffer), typeDiscriminator: "injected")]
public class ShaderStorageBuffer
{
    public required int Set { get; set; }
    public required int Binding { get; set; }
}

public class ConfigurableShaderStorageBuffer : ShaderStorageBuffer
{
    public required string Name { get; set; }
}

public class InjectedShaderStorageBuffer : ShaderStorageBuffer
{
    public required string Identifier { get; set; }
}

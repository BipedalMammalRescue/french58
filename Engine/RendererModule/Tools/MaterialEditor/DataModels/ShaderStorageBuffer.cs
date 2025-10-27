using System.Text.Json.Serialization;

namespace DataModels;

[JsonPolymorphic]
[JsonDerivedType(typeof(ConfigurableShaderStorageBuffer), typeDiscriminator: "configurable")]
[JsonDerivedType(typeof(StaticInjectedShaderStorageBuffer), typeDiscriminator: "injected")]
public class ShaderStorageBuffer
{
    public required int Binding { get; set; }
}

public class ConfigurableShaderStorageBuffer : ShaderStorageBuffer
{
    public required string Name { get; set; }
}

public class StaticInjectedShaderStorageBuffer : ShaderStorageBuffer
{
    public required StaticStorageBufferIdentifier Identifier { get; set; }
}

public class DynamicInjectedShaderStorageBuffer : ShaderStorageBuffer
{
    public required DynamicStorageBufferIdentifier Identifier { get; set; }
}

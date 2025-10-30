using System.Text.Json.Serialization;

namespace DataModels;

[JsonPolymorphic]
[JsonDerivedType(typeof(StaticInjectedShaderStorageBuffer), typeDiscriminator: "static")]
[JsonDerivedType(typeof(DynamicInjectedShaderStorageBuffer), typeDiscriminator: "dynamic")]
public class ShaderStorageBuffer
{
    public required int Binding { get; set; }
}

public class StaticInjectedShaderStorageBuffer : ShaderStorageBuffer
{
    [JsonConverter(typeof(JsonStringEnumConverter))]
    public required StaticStorageBufferIdentifier Identifier { get; set; }
}

public class DynamicInjectedShaderStorageBuffer : ShaderStorageBuffer
{
    [JsonConverter(typeof(JsonStringEnumConverter))]
    public required DynamicStorageBufferIdentifier Identifier { get; set; }
}

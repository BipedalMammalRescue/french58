using System.Text.Json.Serialization;
using EntityBuilder.Abstractions;

namespace DataModels;

[JsonPolymorphic]
[JsonDerivedType(typeof(ConfigurableShaderUniform), typeDiscriminator: "configurable")]
[JsonDerivedType(typeof(DynamicInjectedShaderUniform), typeDiscriminator: "dynamic")]
[JsonDerivedType(typeof(StaticInjectedShaderUniform), typeDiscriminator: "static")]
public abstract class ShaderUniform
{
    public required int Binding { get; set; }
}

public class ConfigurableShaderUniform : ShaderUniform
{
    public required string Name { get; set; }
    public required Variant Default { get; set; }
}

public class DynamicInjectedShaderUniform : ShaderUniform
{
    [JsonConverter(typeof(JsonStringEnumConverter))]
    public required DynamicUniformIdentifier Identifier { get; set; }
}

public class StaticInjectedShaderUniform : ShaderUniform
{
    [JsonConverter(typeof(JsonStringEnumConverter))]
    public required StaticUniformIdentifier Identifier { get; set; }
}

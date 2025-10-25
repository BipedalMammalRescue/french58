using System.Text.Json.Serialization;
using EntityBuilder.Abstractions;

namespace DataModels;

[JsonPolymorphic]
[JsonDerivedType(typeof(ConfigurableShaderUniform), typeDiscriminator: "configurable")]
[JsonDerivedType(typeof(InjectedShaderUniform), typeDiscriminator: "injected")]
public abstract class ShaderUniform
{
    public required int Set { get; set; }
    public required int Binding { get; set; }
}

public class ConfigurableShaderUniform : ShaderUniform
{
    public required string Name { get; set; }
    public required Variant Default { get; set; }
}

public class InjectedShaderUniform : ShaderUniform
{
    public required string Identifier { get; set; }
}
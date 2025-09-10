using System.Text.Json.Serialization;

namespace SignourneyEngine.Companion.DataModels.BuildActions.Components.Input;

[JsonPolymorphic]
[JsonDerivedType(typeof(ConcatInput), typeDiscriminator: "concat")]
[JsonDerivedType(typeof(FileInput), typeDiscriminator: "file")]
public interface IInputComponent
{
    Task TransformAsync(BuildEnvironment environment, Stream source, Stream destination);
}

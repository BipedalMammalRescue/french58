using System.Text.Json.Serialization;

namespace SignourneyEngine.Companion.DataModels.BuildActions.Components.Output;

[JsonPolymorphic]
[JsonDerivedType(typeof(FileOutput), typeDiscriminator: "file")]
public interface IOutputComponent
{
    Task TransformAsync(BuildEnvironment environment, Stream prev, Stream next);
}

using System.Text.Json.Serialization;

namespace SignourneyEngine.Companion.DataModels.BuildActions.Components.Output;

[JsonPolymorphic]
[JsonDerivedType(typeof(FileOutput), typeDiscriminator: "file")]
public interface IOutputComponent
{
    void Transform(BuildEnvironment environment, Stream prev, Stream next);
}

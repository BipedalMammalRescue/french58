using System.Text.Json.Serialization;
using SignourneyEngine.Companion.DataModels.BuildActions.Components.Input;

namespace SignourneyEngine.Companion.DataModels.BuildActions.Components;

public class ConcatInput : IInputComponent
{
    public required IInputComponent[] Sources { get; set; }

    public void Transform(BuildEnvironment environment, Stream source, Stream destination)
    {
        foreach (IInputComponent src in Sources)
        {
            src.Transform(environment, source, destination);
        }
    }
}

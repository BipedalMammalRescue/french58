namespace SignourneyEngine.Companion.DataModels.BuildActions.Components.Input;

/// <summary>
/// Dump input data into a file and drop the original input.
/// </summary>
public class FileInput : IInputComponent
{
    public required string Path { get; set; }

    public void Transform(BuildEnvironment environment, Stream source, Stream destination)
    {
        using FileStream file = File.Create(environment.ExpandValues(Path));
        source.CopyTo(file);
    }
}

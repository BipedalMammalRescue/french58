namespace SignourneyEngine.Companion.DataModels.BuildActions.Components.Output;

public class FileOutput : IOutputComponent
{
    public required string Path { get; set; }

    public void Transform(BuildEnvironment environment, Stream prev, Stream next)
    {
        using FileStream source = File.OpenRead(environment.ExpandValues(Path));
        source.CopyTo(next);
    }
}

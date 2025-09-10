using System.Collections.Immutable;

namespace SignourneyEngine.Companion.DataModels.BuildActions;

public class BuildResult
{
    public ImmutableDictionary<string, string> Tags { get; set; } = ImmutableDictionary<string, string>.Empty;
    public string OutputPath { get; set; } = string.Empty;
    public string[] Errors { get; set; } = [];
}

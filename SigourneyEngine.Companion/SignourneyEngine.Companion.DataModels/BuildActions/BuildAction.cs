using System.Collections.Immutable;
using System.Text.Json.Serialization;
using System.Text.RegularExpressions;
using SignourneyEngine.Companion.DataModels.BuildActions.Components.Input;
using SignourneyEngine.Companion.DataModels.BuildActions.Components.Output;

namespace SignourneyEngine.Companion.DataModels.BuildActions;

public partial record BuildEnvironment(IDictionary<string, string> SourceData, BuildResult[] ChildrenResults)
{
    [GeneratedRegex("#\\((?<fieldref>\\w+)\\)")]
    private partial Regex GetSourceReference();

    [GeneratedRegex("#(?<id>[0-9]+):(?<tag>\\w+)")]
    private partial Regex GetTagReference();

    [GeneratedRegex("#(?<id>[0-9]+)")]
    private partial Regex GetOutputReference();

    public string ExpandValues(string source)
    {
        // expand env var
        string result = Environment.ExpandEnvironmentVariables(source);

        // expand source fields from the asset file
        result = GetSourceReference().Replace(result, match =>
        {
            if (!match.Success)
                return result;

            string field = match.Groups["fieldref"].Value;
            if (!SourceData.TryGetValue(field, out string? foundValue))
                return result;

            return foundValue;
        });

        // expand child tags
        result = GetTagReference().Replace(result, match =>
        {
            int childIndex = int.Parse(match.Groups["id"].Value);
            string tag = match.Groups["tag"].Value;
            return ChildrenResults[childIndex].Tags[tag];
        });

        // expand child results (tags have been taken out already so there should be safe to use a subpattern)
        result = GetOutputReference().Replace(result, match =>
        {
            int childIndex = int.Parse(match.Groups["id"].Value);
            return ChildrenResults[childIndex].OutputPath;
        });

        return result;
    }
}

[JsonPolymorphic]
[JsonDerivedType(typeof(CommandBuildAction), typeDiscriminator: "command")]
public abstract class BuildAction
{
    protected static BuildResult Result(Dictionary<string, string> tags) => new() { Tags = ImmutableDictionary<string, string>.Empty.AddRange(tags) };
    protected static BuildResult Error(params string[] errors) => new() { Errors = errors };

    // transforms the input
    public IInputComponent[] InputComponents { get; set; } = [];
    public IOutputComponent[] OutputComponents { get; set; } = [];

    // used to generate extra tags
    public Dictionary<string, string> Tags { get; set; } = [];

    public (BuildResult Result, string OutputPath) Execute(BuildEnvironment environment)
    {
        HashSet<string> tempFilesUsed = [];

        // transform the input
        Stream input = Stream.Null;
        foreach (IInputComponent inputComponent in InputComponents)
        {
            // make sure every file used for input is automatically collected
            string tempFile = Path.GetTempFileName();
            tempFilesUsed.Add(tempFile);

            // transform data
            Stream nextInput = File.Create(tempFile);
            inputComponent.Transform(environment, input, nextInput);

            // swap
            input.Dispose();
            input = nextInput;

            // seek back to beginning for the resulting new input
            input.Seek(0, SeekOrigin.Begin);
        }

        // dump output into a temp file
        string outputPath = Path.GetTempFileName();
        FileStream output = File.Create(outputPath);
        BuildResult result = ExecuteCore(environment, input, output);

        // close the files used just now
        input.Dispose();
        output.Dispose();

        // transform the output
        foreach (IOutputComponent outputComponent in OutputComponents)
        {
            // get a new file
            string nextOutputPath = Path.GetTempFileName();

            // funnel the output to the next file
            using FileStream prevOutput = File.OpenRead(outputPath);
            using FileStream nextOutput = File.Create(outputPath);
            outputComponent.Transform(environment, prevOutput, nextOutput);

            // keep tap and swap
            tempFilesUsed.Add(outputPath);
            outputPath = nextOutputPath;
        }

        // clean up
        foreach (string tempFile in tempFilesUsed)
        {
            if (File.Exists(tempFile))
            {
                File.Delete(tempFile);
            }
        }

        // add in the extra tags
        foreach (KeyValuePair<string, string> extraTag in Tags)
        {
            result.Tags = result.Tags.Add(environment.ExpandValues(extraTag.Key), environment.ExpandValues(extraTag.Value));
        }

        return (result, outputPath);
    }

    /// <summary>
    /// Implement this method for a type of build action. 
    /// </summary>
    /// <param name="input">The finalized input to this task.</param>
    /// <param name="output">The initial output of this task.</param>
    /// <param name="environment">Helper functions that's based on part of the input data.</param>
    /// <returns></returns>
    protected abstract BuildResult ExecuteCore(BuildEnvironment environment, Stream input, Stream output);
}

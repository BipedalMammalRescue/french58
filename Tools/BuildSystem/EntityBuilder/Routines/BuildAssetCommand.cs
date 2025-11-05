using System.Collections.Immutable;
using System.CommandLine;
using System.CommandLine.Invocation;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using EntityBuilder.Abstractions;
using MuThr.DataModels;
using MuThr.DataModels.BuildActions;
using MuThr.DataModels.Diagnostic;
using MuThr.Sdk;

namespace EntityBuilder.Routines;

public class BuildAssetCommand : Command
{
    private static readonly Option<DirectoryInfo> s_recipesRootOption = new("--recipe-root")
    {
        Description = "Path to build recipes. Default is \"./recipes\".",
    };

    private static readonly Option<DirectoryInfo> s_assetOutOption = new("--asset-out")
    {
        Description = "Directory to put all built assets."
    };

    private static readonly Option<string> s_assetFileExtensionOption = new("--asset-file-extension")
    {
        Description = "File extension of each individual built asset. Default is \"*.bse_asset\".",
    };

    private static readonly Argument<FileInfo> s_inputFileArgument = new("asset-file", "Input asset file.");

    private readonly IMuThrLogger _logger;

    public BuildAssetCommand(IMuThrLogger logger) : base("build-asset")
    {
        _logger = logger;

        AddOption(s_recipesRootOption);
        AddOption(s_assetOutOption);
        AddOption(s_assetFileExtensionOption);
        AddArgument(s_inputFileArgument);

        this.SetHandler(async context => context.ExitCode = await InvokeAsync(context).ConfigureAwait(false));
    }

    private static async Task<Recipe?> LoadRecipe(string filePath)
    {
        try
        {
            await using FileStream fs = File.OpenRead(filePath);
            return JsonSerializer.Deserialize<Recipe>(fs);
        }
        catch
        {
            return null;
        }
    }

    public async Task<int> InvokeAsync(InvocationContext context)
    {
        // for debug purposes
        _logger.Information("Using asset root directory: {root}", Environment.CurrentDirectory);

        // sanity check input file
        FileInfo inputFile = context.ParseResult.GetValueForArgument(s_inputFileArgument);
        if (!File.Exists(inputFile.FullName))
        {
            _logger.Error("Invalid input path: {path}", inputFile);
            return (int)ErrorCodes.InvalidInputFile;
        }
        _logger.Information("Input path: {path}", inputFile);

        // check file extension is valid
        string assetFileExtension = context.ParseResult.GetValueForOption(s_assetFileExtensionOption) ?? "bse_asset";
        try
        {
            FileStream fs = File.Create($"{Guid.NewGuid()}.{assetFileExtension}");
            await fs.DisposeAsync().ConfigureAwait(false);
            File.Delete(fs.Name);
        }
        catch
        {
            _logger.Error("Invalid file extension option: {ext}", assetFileExtension);
            return (int)ErrorCodes.InvalidFileExtension;
        }
        _logger.Information("Asset extension: {ext}", assetFileExtension);

        // check output folder is valid
        DirectoryInfo assetOut = context.ParseResult.GetValueForOption(s_assetOutOption) ?? new DirectoryInfo("./built_assets");
        try
        {
            Directory.CreateDirectory(assetOut.FullName);
        }
        catch
        {
            _logger.Error("Invalid output directory: {output}", assetOut.FullName);
            return (int)ErrorCodes.InvalidAssetOutput;
        }

        // load recipes
        DirectoryInfo recipeRoot = context.ParseResult.GetValueForOption(s_recipesRootOption) ?? new DirectoryInfo("./recipes");
        if (!Directory.Exists(recipeRoot.FullName))
        {
            _logger.Error("Invalid recipe root directory: {recipeRoot}", recipeRoot.FullName);
            return (int)ErrorCodes.InvalidRecipeRoot;
        }
        _logger.Information("Recipe root: {root}", recipeRoot.FullName);

        ImmutableDictionary<string, ImmutableDictionary<string, BuildAction>> recipes =
            (await Task.WhenAll(Directory
                .EnumerateFiles(recipeRoot.FullName, "*.se_recipe", SearchOption.AllDirectories)
                .Select(LoadRecipe)))
            .Where(x => x is not null)
            .Select(x => x!)
            .GroupBy(x => x.Module)
            .Select(group => (group.Key, group.ToImmutableDictionary(x => x.Type, x => x.Action)))
            .ToImmutableDictionary(x => x.Key, x => x.Item2);
        _logger.Information("Loaded {count} recipes.", recipes.Sum(x => x.Value.Count));

        // initialize coordinator
        Coordinator coordinator = new(new AssetTaskProvider(recipes), _logger.WithChannel("Coordinator", "AssetBuilder"));
        _logger.Information("Coordinator initialized.");

        // schedul initial task
        coordinator.ScheduleTask(Path.GetRelativePath(Environment.CurrentDirectory, inputFile.FullName));

        // collect output
        ImmutableDictionary<string, BuildResult> buildOutput = await coordinator.OutputTask.ConfigureAwait(false);
        foreach (var output in buildOutput)
        {
            if (output.Value.Errors.Length > 0)
            {
                foreach (var err in output.Value.Errors)
                {
                    _logger.Error("Task error ({key}): {err}", output.Key, err);
                }
            }
            else
            {
                string assetFileName = Path.ChangeExtension(Convert.ToHexString(MD5.HashData(Encoding.UTF8.GetBytes(output.Key))), assetFileExtension);
                string outFilePath = Path.Combine(assetOut.FullName, assetFileName);
                File.Move(output.Value.OutputPath, outFilePath, overwrite: true);
                _logger.Information("Task result <{key}> {path}", output.Key, assetFileName);
            }
        }

        return 0;
    }
}

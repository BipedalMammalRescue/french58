using System.Collections.Immutable;
using System.CommandLine;
using System.CommandLine.Parsing;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using EntityBuilder;
using EntityBuilder.Abstractions;
using MuThr.DataModels;
using MuThr.DataModels.BuildActions;
using MuThr.DataModels.Diagnostic;
using MuThr.Sdk;
using Serilog;

internal enum ErrorCodes : int
{
    InvalidInputFile,
    InvalidFileExtension,
    InvalidRecipeRoot,
    InvalidAssetOutput
}

// TODO: how to deal with asset root? not a good idea to use relative paths

internal class Program
{
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

    private static async Task<int> Main(string[] args)
    {
        // initialize logger
#pragma warning disable CA1859 // Use concrete types when possible for improved performance
        IMuThrLogger logger = new MuThrLogger(["AppRoot"], new LoggerConfiguration()
#pragma warning restore CA1859 // Use concrete types when possible for improved performance
            .Enrich.With(new MuThrLogEnricher())
            .Enrich.FromLogContext()
            .WriteTo.Console(outputTemplate: "[{Timestamp:HH:mm:ss} {Level:u3}][{PrimaryChannel}] {Message:lj}{NewLine}{Exception}")
            .CreateLogger());

        logger.Information("Initializing entity builder ...");

        // command line utilities
        Option<DirectoryInfo> recipesRootOption = new("--recipes-root")
        {
            Description = "Path to build recipes. Default is \"./recipes\".",
        };

        Option<DirectoryInfo> assetOutOption = new("--asset-out")
        {
            Description = "Directory to put all built assets."
        };

        Option<string> assetFileExtensionOption = new("--asset-file-extension")
        {
            Description = "File extension of each individual built asset. Default is \"*.se_bin\".",
        };

        Argument<string> inputFileArgument = new("asset-file", "Input asset file.");

        RootCommand rootCommand = new("Entity Builder: a Sigourney Engine companion application")
        {
            inputFileArgument,
            recipesRootOption,
            assetOutOption,
            assetFileExtensionOption,
        };

        ParseResult results = rootCommand.Parse(args);

        // sanity check input file
        string? inputFile = results.GetValueForArgument(inputFileArgument);
        if (inputFile is null)
        {
            logger.Error("Input path argument is required.");
            return (int)ErrorCodes.InvalidInputFile;
        }
        else if (!File.Exists(inputFile))
        {
            logger.Error("Invalid input path: {path}", inputFile);
            return (int)ErrorCodes.InvalidInputFile;
        }
        logger.Information("Input path: {path}", inputFile);

        // check file extension is valid
        string assetFileExtension = results.GetValueForOption(assetFileExtensionOption) ?? "se_bin";
        try
        {
            FileStream fs = File.Create($"{Guid.NewGuid()}.{assetFileExtension}");
            await fs.DisposeAsync().ConfigureAwait(false);
            File.Delete(fs.Name);
        }
        catch
        {
            logger.Error("Invalid file extension option: {ext}", assetFileExtension);
            return (int)ErrorCodes.InvalidFileExtension;
        }
        logger.Information("Asset extension: {ext}", assetFileExtension);

        // check output folder is valid
        DirectoryInfo assetOut = results.GetValueForOption(assetOutOption) ?? new DirectoryInfo("./built_assets");
        try
        {
            Directory.CreateDirectory(assetOut.FullName);
        }
        catch
        {
            logger.Error("Invalid output directory: {output}", assetOut.FullName);
            return (int)ErrorCodes.InvalidAssetOutput;
        }

        // load recipes
        DirectoryInfo recipeRoot = results.GetValueForOption(recipesRootOption) ?? new DirectoryInfo("./recipes");
        if (!Directory.Exists(recipeRoot.FullName))
        {
            logger.Error("Invalid recipe root directory: {recipeRoot}", recipeRoot.FullName);
            return (int)ErrorCodes.InvalidRecipeRoot;
        }
        logger.Information("Recipe root: {root}", recipeRoot.FullName);

        ImmutableDictionary<string, ImmutableDictionary<string, BuildAction>> recipes =
            (await Task.WhenAll(Directory
                .EnumerateFiles(recipeRoot.FullName, "*.se_recipe", SearchOption.AllDirectories)
                .Select(LoadRecipe)))
            .Where(x => x is not null)
            .Select(x => x!)
            .GroupBy(x => x.Module)
            .Select(group => (group.Key, group.ToImmutableDictionary(x => x.Type, x => x.Action)))
            .ToImmutableDictionary(x => x.Key, x => x.Item2);
        logger.Information("Loaded {count} recipes.", recipes.Sum(x => x.Value.Count));

        // initialize coordinator
        Coordinator coordinator = new(new AssetTaskProvider(recipes), logger.WithChannel("AssetBuilder"));
        logger.Information("Coordinator initialized.");

        coordinator.ScheduleTask(inputFile);
        ImmutableDictionary<string, BuildResult> buildOutput = await coordinator.OutputTask.ConfigureAwait(false);
        foreach (var output in buildOutput)
        {
            if (output.Value.Errors.Length > 0)
            {
                foreach (var err in output.Value.Errors)
                {
                    logger.Error("Task error ({key}): {err}", output.Key, err);
                }
            }
            else
            {
                string assetFileName = Path.ChangeExtension(BitConverter.ToString(MD5.HashData(Encoding.UTF8.GetBytes(output.Key))).Replace("-", string.Empty), assetFileExtension);
                string outFilePath = Path.Combine(assetOut.FullName, assetFileName);
                File.Move(output.Value.OutputPath, outFilePath);
                logger.Information("Task result ({key}): {path}", output.Key, outFilePath);
            }
        }

        return 0;
    }
}

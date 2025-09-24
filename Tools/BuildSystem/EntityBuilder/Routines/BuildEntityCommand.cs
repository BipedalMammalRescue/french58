using System.Buffers.Binary;
using System.Collections.Immutable;
using System.CommandLine;
using System.CommandLine.Invocation;
using System.Diagnostics;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using CommunityToolkit.HighPerformance;
using EntityBuilder.Abstractions;
using MuThr.DataModels;
using MuThr.DataModels.BuildActions;
using MuThr.DataModels.Diagnostic;
using MuThr.Sdk;

namespace EntityBuilder.Routines;

public class BuildEntityCommand : Command
{
    private static readonly Argument<FileInfo> s_inputFileArgument = new("asset-file", "Input asset file.");

    private static readonly Option<DirectoryInfo> s_recipesRootOption = new("--recipe-root")
    {
        Description = "Path to build recipes. Default is \"./recipes\".",
    };

    private static readonly Option<DirectoryInfo> s_outOption = new("--out")
    {
        Description = "Directory to put all built entities and assets."
    };

    private readonly IMuThrLogger _logger;

    public BuildEntityCommand(IMuThrLogger logger) : base("build-entity")
    {
        _logger = logger;

        AddOption(s_outOption);
        AddOption(s_recipesRootOption);
        AddArgument(s_inputFileArgument);
        this.SetHandler(async context => context.ExitCode = await InvokeAsync(context).ConfigureAwait(false));
    }
    
    private static async Task<Recipe?> LoadRecipeAsync(string filePath)
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

    private async Task<Entity?> LoadEntityAsync(string path)
    {
        try
        {
            await using FileStream fs = File.OpenRead(path);
            Entity? entity = await JsonSerializer.DeserializeAsync<Entity>(fs).ConfigureAwait(false);
            return entity;
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to deserlialize entity from {path}", path);
            return null;
        }
    }

    private class AssetGroup
    {
        public required string Module { get; set; }
        public required string Type { get; set; }
        public required IEnumerable<string> Tasks { get; set; }
        public IEnumerable<string> Roots { get; set; } = [];
        public IEnumerable<AssetGroup> DependsOn { get; set; } = [];

        public override string ToString() => $"{Module}:{Type}";
    }

    private static IEnumerable<AssetGroup> BuildAssetGroups(AssetTaskProvider provider)
    {
        foreach (KeyValuePair<string, ImmutableDictionary<string, ImmutableList<string>>> moduleGroup in provider.AssetTable)
        {
            foreach (KeyValuePair<string, ImmutableList<string>> assetGroup in moduleGroup.Value)
            {
                yield return new AssetGroup()
                {
                    Module = moduleGroup.Key,
                    Type = assetGroup.Key,
                    Tasks = assetGroup.Value
                };
            }
        }
    }

    private async Task<string?> BuildComponentGroupAsync(ComponentGroup group)
    {
        using Process componentBuilder = new();
        componentBuilder.StartInfo.FileName = "BuildComponent";
        componentBuilder.StartInfo.RedirectStandardInput = true;
        componentBuilder.StartInfo.RedirectStandardOutput = true;
        componentBuilder.ErrorDataReceived += (_, e) =>
        {
            _logger.Information("Builder message: {message}", e.Data);
        };

        // TODO: set up a way to report error
        componentBuilder.StartInfo.RedirectStandardError = true;

        componentBuilder.Start();

        Task inputTask = Task.Run(() =>
        {
            Span<byte> variantBuffer = stackalloc byte[72];

            // module
            componentBuilder.StandardInput.BaseStream.Write(MD5.HashData(Encoding.UTF8.GetBytes(group.Module)));

            // type
            componentBuilder.StandardInput.BaseStream.Write(MD5.HashData(Encoding.UTF8.GetBytes(group.Type)));

            // component count
            componentBuilder.StandardInput.BaseStream.Write(group.Components.Length);

            foreach (CompiledComponent component in group.Components)
            {
                // ids
                componentBuilder.StandardInput.BaseStream.Write(component.Id);
                componentBuilder.StandardInput.BaseStream.Write(component.Entity);

                // fields
                componentBuilder.StandardInput.BaseStream.Write(component.Fields.Length);
                foreach (ComponentField field in component.Fields)
                {
                    componentBuilder.StandardInput.BaseStream.Write(MD5.HashData(Encoding.UTF8.GetBytes(field.Name)));

                    variantBuffer.Clear();
                    BinaryPrimitives.WriteInt32LittleEndian(variantBuffer, (byte)field.Value.Type);

                    // align the data to 8 bytes
                    Span<byte> dataBuffer = variantBuffer[8..];
                    field.Value.Write(dataBuffer);
                    componentBuilder.StandardInput.BaseStream.Write(variantBuffer);
                }
            }
        });

        string tempFilePath = Path.GetTempFileName();
        {

            await using FileStream outFile = File.Create(tempFilePath);
            Task copyTask = componentBuilder.StandardOutput.BaseStream.CopyToAsync(outFile);

            await inputTask.ConfigureAwait(false);
            await componentBuilder.WaitForExitAsync().ConfigureAwait(false);
            await copyTask.ConfigureAwait(false);
        }

        if (componentBuilder.ExitCode != 0)
        {
            File.Delete(tempFilePath);
            return null;
        }

        return tempFilePath;
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

        // try to deserialize the file
        Entity? loadedEntity = await LoadEntityAsync(inputFile.FullName).ConfigureAwait(false);
        if (loadedEntity == null)
        {
            _logger.Error("Failed to load entity.");
            return (int)ErrorCodes.InvalidEntity;
        }

        // check output folder is valid
        DirectoryInfo outPath = context.ParseResult.GetValueForOption(s_outOption) ?? new DirectoryInfo("./built_data");
        try
        {
            Directory.CreateDirectory(outPath.FullName);
        }
        catch
        {
            _logger.Error("Invalid output directory: {output}", outPath.FullName);
            return (int)ErrorCodes.InvalidAssetOutput;
        }

        // load asset recipes
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
                .Select(LoadRecipeAsync)))
            .Where(x => x is not null)
            .Select(x => x!)
            .GroupBy(x => x.Module)
            .Select(group => (group.Key, group.ToImmutableDictionary(x => x.Type, x => x.Action)))
            .ToImmutableDictionary(x => x.Key, x => x.Item2);
        _logger.Information("Loaded {count} recipes.", recipes.Sum(x => x.Value.Count));

        // initialize coordinator
        var taskProvider = new AssetTaskProvider(recipes);
        Coordinator coordinator = new(taskProvider, _logger.WithChannel("AssetBuilder", "Coordinator"));
        _logger.Information("Coordinator initialized.");

        // build all entities
        _logger.Verbose("Compiling entities ...");
        (CompiledEntity[] entities, ComponentGroup[] componentGroups) = loadedEntity.Compile(coordinator);
        _logger.Information("Entities compile finished: {eCount} entities + {cCount} components.", entities.Length, componentGroups.Length);

        // collect asset output
        ImmutableDictionary<string, BuildResult> buildOutput = await coordinator.OutputTask.ConfigureAwait(false);
        _logger.Information("Assets built.");

        // copy the built assets to the output folder
        Task copyTask = Task.Run(() =>
        {
            foreach (KeyValuePair<string, BuildResult> output in buildOutput)
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
                    string assetFileName = Path.ChangeExtension(Convert.ToHexString(MD5.HashData(Encoding.UTF8.GetBytes(output.Key))), "bse_asset");
                    string outFilePath = Path.Combine(outPath.FullName, assetFileName);
                    File.Move(output.Value.OutputPath, outFilePath, overwrite: true);
                    _logger.Information("Task result <{key}> {path}", output.Key, assetFileName);
                }
            }
            _logger.Information("Built assets copied to {outPath}", outPath.FullName);
        });

        // build asset groups
        _logger.Verbose("Building asset groups ...");
        AssetGroup[] assetGroups = [.. BuildAssetGroups(taskProvider)];
        Dictionary<string, AssetGroup> taskToGroupMapping = assetGroups.SelectMany(g => g.Tasks.Select(t => new KeyValuePair<string, AssetGroup>(t, g))).ToDictionary();
        foreach (AssetGroup group in assetGroups)
        {
            group.DependsOn = group.Tasks.SelectMany(task => taskProvider.AssetRelations[task]).Where(buildOutput.ContainsKey).Select(t => taskToGroupMapping[t]).ToHashSet();
            group.Roots = group.Tasks.Where(t => taskProvider.AssetRelations[t].All(dep => !taskToGroupMapping.TryGetValue(dep, out AssetGroup? depGroup) || depGroup != group));
        }

        // sort asset groups
        _logger.Verbose("Sorting asset groups in topolotical order ...");
        List<AssetGroup> groupOrdering = [];
        {
            HashSet<AssetGroup> seen = [];
            Stack<(AssetGroup, ImmutableHashSet<AssetGroup>)> stack = [];
            foreach (AssetGroup group in assetGroups)
            {
                stack.Push((group, []));
            }

            while (stack.Count > 0)
            {
                (AssetGroup next, ImmutableHashSet<AssetGroup> path) = stack.Pop();
                if (path.Contains(next))
                {
                    _logger.Fatal("Circular dependency in asset groups detected: {path} - {last}", path, next);
                    return (int)ErrorCodes.CircularDependency;
                }

                ImmutableHashSet<AssetGroup> newPath = path.Add(next);
                foreach (AssetGroup child in next.DependsOn)
                {
                    stack.Push((child, newPath));
                }

                if (seen.Add(next))
                {
                    groupOrdering.Add(next);
                }
            }
        }

        // sort inside each group
        _logger.Verbose("Sorting within each asset group in topological order ...");
        foreach (AssetGroup group in groupOrdering)
        {
            HashSet<string> seen = [];
            List<string> outOrder = [];
            Stack<(string, ImmutableHashSet<string>)> stack = [];

            if (!group.Roots.Any())
            {
                _logger.Fatal("Circular dependency within asset group ({group}) detected.", group);
                return (int)ErrorCodes.CircularDependency;
            }

            foreach (string task in group.Roots)
            {
                stack.Push((task, []));
            }

            while (stack.Count > 0)
            {
                (string next, ImmutableHashSet<string> path) = stack.Pop();
                if (path.Contains(next))
                {
                    _logger.Fatal("Circular dependency within asset group ({group}) detected: {path} - {last}", group, path, next);
                    return (int)ErrorCodes.CircularDependency;
                }

                if (seen.Add(next))
                {
                    outOrder.Add(next);
                }
            }

            group.Tasks = outOrder;
        }

        // print out the asset list
        _logger.Verbose("Writing out asset groups ...");
        await using FileStream outEntityFile = File.Create(Path.Combine(outPath.FullName, Path.ChangeExtension(inputFile.Name, "bse_entity")));
        outEntityFile.Write(0xCCBBFFF1);
        outEntityFile.Write(assetGroups.Length);
        foreach (AssetGroup group in assetGroups)
        {
            outEntityFile.Write(MD5.HashData(Encoding.UTF8.GetBytes(group.Module)));
            outEntityFile.Write(MD5.HashData(Encoding.UTF8.GetBytes(group.Type)));
            outEntityFile.Write(group.Tasks.Count());
            foreach (string task in group.Tasks)
            {
                outEntityFile.Write(MD5.HashData(Encoding.UTF8.GetBytes(task)));
            }
        }
        _logger.Information("Assets section printed.");

        // print out the entities
        _logger.Verbose("Building entities ...");
        outEntityFile.Write(0xCCBBFFF2);
        outEntityFile.Write(entities.Length);
        foreach (CompiledEntity entity in entities)
        {
            outEntityFile.Write(entity.Id);
            outEntityFile.Write(entity.Parent);
        }
        _logger.Information("Entity section printed.");

        // launch the component builder and start funneling in the data
        _logger.Verbose("Launching component builder processes ...");
        outEntityFile.Write(0xCCBBFFF3);
        (ComponentGroup, Task<string?>)[] componentTasks = [.. componentGroups.Select(group => (group, Task.Run(async () => await BuildComponentGroupAsync(group).ConfigureAwait(false))))];
        outEntityFile.Write(componentTasks.Length);
        foreach ((ComponentGroup group, Task<string?> task) in componentTasks)
        {
            string? result = await task.ConfigureAwait(false);
            if (result == null)
            {
                _logger.Error("Failed to compile components for group {module}:{type}, it's skipped in the result entity.", group.Module, group.Type);
                continue;
            }

            outEntityFile.Write(MD5.HashData(Encoding.UTF8.GetBytes(group.Module)));
            outEntityFile.Write(MD5.HashData(Encoding.UTF8.GetBytes(group.Type)));

            outEntityFile.Write(group.Components.Length);

            await using FileStream componentFile = File.OpenRead(result);
            await componentFile.CopyToAsync(outEntityFile);

            _logger.Information("Printed component group {module}:{type}", group.Module, group.Type);
        }
        _logger.Information("Component section printed.");
        return 0;
    }
}

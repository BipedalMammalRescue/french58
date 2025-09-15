using System.Collections.Immutable;
using System.Text.Json;
using EntityBuilder.Abstractions;
using MuThr.DataModels.BuildActions;
using MuThr.DataModels.Schema;
using MuThr.Sdk;

namespace EntityBuilder;

public class AssetTaskProvider(ImmutableDictionary<string, ImmutableDictionary<string, BuildAction>> recipes) : ITaskProvider
{
    public (BuildAction Action, IDataPoint SourceData) CreateTask(string key)
    {
        using FileStream assetFile = File.OpenRead(key);
        Asset asset = JsonSerializer.Deserialize<Asset>(assetFile) ?? throw new Exception($"Can't find valid asset in input file: {key}");
        if (!recipes.TryGetValue(asset.Module, out var perModuleRecipes) || !perModuleRecipes.TryGetValue(asset.Type, out BuildAction? action))
            throw new Exception($"Can't find recipe for asset type {asset.Module}:{asset.Type}");

        return (action, asset.Data);
    }
}

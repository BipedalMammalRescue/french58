using System.Collections.Concurrent;
using System.Collections.Immutable;
using System.Text.Json;
using EntityBuilder.Abstractions;
using MuThr.DataModels.BuildActions;
using MuThr.DataModels.Schema;
using MuThr.Sdk;

namespace EntityBuilder;

public class AssetTaskProvider(ImmutableDictionary<string, ImmutableDictionary<string, BuildAction>> recipes) : ITaskProvider
{
    // record some information when the asset is deserialized
    public ConcurrentDictionary<string, IEnumerable<string>> AssetRelations = [];
    public ConcurrentDictionary<string, ImmutableDictionary<string, ImmutableList<string>>> AssetTable = [];

    public (BuildAction Action, IDataPoint SourceData) CreateTask(string key)
    {
        // requires asset files to use paths relative to the asset folder
        using FileStream assetFile = File.OpenRead(Path.Combine(key));

        // load asset and update the table
        Asset asset = JsonSerializer.Deserialize<JsonAsset>(assetFile)?.ToAsset() ?? throw new Exception($"Can't find valid asset in input file: {key}");
        AssetRelations.TryAdd(key, asset.Data.AssetReferences);
        AssetTable.AddOrUpdate(asset.Module, ImmutableDictionary<string, ImmutableList<string>>.Empty.Add(asset.Type, [key]), (_, old) =>
        {
            if (old.TryGetValue(asset.Type, out ImmutableList<string>? existing))
                return old.Remove(asset.Type).Add(asset.Type, existing.Add(key));

            return old.Add(asset.Type, [key]);
        });

        if (!recipes.TryGetValue(asset.Module, out var perModuleRecipes) || !perModuleRecipes.TryGetValue(asset.Type, out BuildAction? action))
            throw new Exception($"Can't find recipe for asset type {asset.Module}:{asset.Type}");

        return (action, asset.Data);
    }
}

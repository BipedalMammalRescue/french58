using System.Text.Json.Serialization;
using MuThr.DataModels.Schema;

namespace EntityBuilder.Abstractions;

public class Asset
{
    public required string Module { get; set; }
    public required string Type { get; set; }
    public required IAssetData Data { get; set; }
}

[JsonPolymorphic]
[JsonDerivedType(typeof(AssetArrayData), typeDiscriminator: "arr")]
[JsonDerivedType(typeof(AssetObjData), typeDiscriminator: "obj")]
[JsonDerivedType(typeof(AssetLeafData), typeDiscriminator: "leaf")]
public interface IAssetData : IDataPoint
{
    IEnumerable<string> AssetReferences { get; }
}

public class AssetArrayData : IAssetData, IArrayDataPoint
{
    public required IAssetData[] Data { get; set; }

    public IEnumerable<string> AssetReferences => Data.SelectMany(d => d.AssetReferences);

    public IDataPoint[] Get() => [.. Data];
}

public class AssetObjData : Dictionary<string, IAssetData>, IAssetData, IObjDataPoint
{
    public IEnumerable<string> AssetReferences => Values.SelectMany(d => d.AssetReferences);

    public IDataPoint? Get(string path) => TryGetValue(path, out IAssetData? data) ? data : null;
}

public class AssetLeafData : Variant, IAssetData
{
    public IEnumerable<string> AssetReferences => Type switch
    {
        VariantType.Path => [Path],
        _ => []
    };
}

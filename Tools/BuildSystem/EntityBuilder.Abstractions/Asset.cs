using System.Text.Json;
using System.Text.Json.Nodes;
using MuThr.DataModels.Schema;

namespace EntityBuilder.Abstractions;

public class JsonAsset
{
    public required string Module { get; set; }
    public required string Type { get; set; }
    public required JsonNode Data { get; set; }

    private static IAssetData TryDecodeJsonObject(JsonObject jsonObj)
    {
        try
        {
            Variant? possibleVariant = jsonObj.Deserialize<Variant>();
            if (possibleVariant != null && possibleVariant.Type != VariantType.Invalid)
                return new AssetLeafData(possibleVariant);
        }
        catch {}

        return new AssetObjData(jsonObj.Select(pair => new KeyValuePair<string, IAssetData>(pair.Key, DecodeJsonData(pair.Value))).ToDictionary());
    }

    private static IAssetData DecodeJsonData(JsonNode? data)
    {
        if (data == null)
            throw new Exception("Encountered NULL value in asset data section!");

        return data.GetValueKind() switch
        {
            JsonValueKind.Object => TryDecodeJsonObject(data.AsObject()),
            JsonValueKind.Array => new AssetArrayData([.. data.AsArray().Select(DecodeJsonData)]),
            _ => throw new Exception($"Encountered unrecognized value in asset data section, path = {data.GetPath()}")
        };
    }

    public Asset ToAsset()
    {
        IAssetData dataSection = DecodeJsonData(Data);
        return new Asset()
        {
            Module = Module,
            Type = Type,
            Data = dataSection
        };
    }
}

public class Asset
{
    public required string Module { get; set; }
    public required string Type { get; set; }
    public required IAssetData Data { get; set; }
}

public interface IAssetData : IDataPoint
{
    IEnumerable<string> AssetReferences { get; }
}

public record AssetArrayData(IAssetData[] Data) : IAssetData, IArrayDataPoint
{
    public IEnumerable<string> AssetReferences => Data.SelectMany(d => d.AssetReferences);

    public IDataPoint[] Get() => [.. Data];
}

public record AssetObjData(Dictionary<string, IAssetData> KvPairs) : IAssetData, IObjDataPoint
{
    public IEnumerable<string> AssetReferences => KvPairs.Values.SelectMany(d => d.AssetReferences);

    public IDataPoint? Get(string path) => KvPairs.TryGetValue(path, out IAssetData? data) ? data : null;
}

public record AssetLeafData(Variant Data) : IAssetData, ILeafDataPoint
{
    public IEnumerable<string> AssetReferences => Data.Type switch
    {
        VariantType.Path => [Data.Path],
        _ => []
    };

    public byte[] GetBytes() => Data.GetBytes();

    public string GetString() => Data.GetString();
}

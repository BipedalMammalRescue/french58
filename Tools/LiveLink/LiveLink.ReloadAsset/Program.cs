using LiveLink.Abstractions;
using EntityBuilder.Abstractions;
using System.Text.Json;
using System.Security.Cryptography;
using System.Text;

internal class Program
{
    // TODO: use a more serious command line setup since we'll need to configure a bunch of relative paths
    private static int Main(string[] args)
    {
        if (args.Length < 1)
        {
            Console.WriteLine("Reloading assets requires at least one argument: file path.");
            return 1;
        }

        string sourceFile = args[0];

        if (!File.Exists(sourceFile))
        {
            Console.WriteLine("Asset file does not exist.");
            return 2;
        }

        try
        {
            using FileStream inputfs = File.OpenRead(sourceFile);
            Asset? asset = JsonSerializer.Deserialize<Asset>(inputfs);
            if (asset == null)
            {
                Console.WriteLine("Invalid asset file in input.");
                return 3;
            }

            byte[] moduleName = MD5.HashData(Encoding.UTF8.GetBytes(asset.Module));
            byte[] assetType = MD5.HashData(Encoding.UTF8.GetBytes(asset.Type));
            byte[] assetId = MD5.HashData(Encoding.UTF8.GetBytes(Path.GetRelativePath("Assets", sourceFile)));

            byte[] payload = [(byte)PacketType.HotReload, ..moduleName, ..assetType, ..assetId];

            using LiveLinkConnection connection = new();
            if (!connection.Connect())
            {
                Console.WriteLine("Failed to connect to game.");
                return 4;   
            }

            connection.Send(payload);
            return 0;
        }
        catch (Exception ex)
        {
            Console.WriteLine("Failed to send asset reload request to game due to exception.");
            Console.WriteLine(ex);
            return -1;
        }
    }
}
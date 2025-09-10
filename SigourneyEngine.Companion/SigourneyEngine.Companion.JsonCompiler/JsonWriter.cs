using System.Buffers.Binary;
using System.Security.Cryptography;
using System.Text;
using SignourneyEngine.Companion.DataModels;

namespace SigourneyEngine.Companion.JsonCompiler;

public static class JsonWriter
{
    private static byte[] SerializeVector(int length, string source)
    {
        string[] segments = [.. source.Split(" ").Where(x => x.Length > 0)];
        if (segments.Length != length)
            throw new Exception("vector shape mismatch");
        float[] results = [.. segments.Select(float.Parse)];
        byte[] result = new byte[results.Length * 4];
        for (int i = 0; i < results.Length; i++)
        {
            BinaryPrimitives.WriteSingleLittleEndian(result.AsSpan(i * 4, 4), results[i]);
        }
        return result;
    }

    public static byte[] ToBinary(this TypedProperty property)
    {
        return property.Type switch
        {
            DataType.Byte => bool.Parse(property.Value) ? [1] : [0],
            DataType.Int32 => BitConverter.GetBytes(int.Parse(property.Value)),
            DataType.Int64 => BitConverter.GetBytes(long.Parse(property.Value)),
            DataType.Uint32 => BitConverter.GetBytes(uint.Parse(property.Value)),
            DataType.Uint64 => BitConverter.GetBytes(ulong.Parse(property.Value)),
            DataType.Float => BitConverter.GetBytes(float.Parse(property.Value)),
            DataType.Vec2 => SerializeVector(2, property.Value),
            DataType.Vec3 => SerializeVector(3, property.Value),
            DataType.Vec4 => SerializeVector(4, property.Value),
            DataType.Mat2 => SerializeVector(4, property.Value),
            DataType.Mat3 => SerializeVector(9, property.Value),
            DataType.Mat4 => SerializeVector(16, property.Value),
            DataType.Path => Path.Exists(property.Value) ? MD5.HashData(Encoding.UTF8.GetBytes(property.Value)) : throw new Exception("asset path doesn't exist"),
            _ => throw new Exception("unsupported datatype"),
        };
    }
}

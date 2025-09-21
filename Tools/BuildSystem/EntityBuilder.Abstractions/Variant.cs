using System.Buffers.Binary;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json.Serialization;
using MuThr.DataModels.Schema;

namespace EntityBuilder.Abstractions;

public enum VariantType : byte
{
    Byte,
    Int32,
    Int64,
    Uint32,
    Uint64,
    Float,
    Vec2,
    Vec3,
    Vec4,
    Mat2,
    Mat3,
    Mat4,
    Path,
    Invalid
};

public class Variant : ILeafDataPoint
{
    [JsonIgnore]
    public VariantType Type { get; private set; } = VariantType.Invalid;

    private object? _internalObj = null;
    private object InternalObj => _internalObj ?? throw new Exception("Variant uninitialized.");

    public byte Byte
    {
        get => (byte)InternalObj; set
        {
            if (_internalObj != null)
                throw new Exception("Multiple values defined for variant.");

            Type = VariantType.Byte;
            _internalObj = value;
        }
    }

    public int Int32
    {
        get => (int)InternalObj; set
        {
            if (_internalObj != null)
                throw new Exception("Multiple values defined for variant.");
            Type = VariantType.Int32;
            _internalObj = value;
        }
    }

    public long Int64
    {
        get => (long)InternalObj; set
        {
            if (_internalObj != null)
                throw new Exception("Multiple values defined for variant.");
            Type = VariantType.Int64;
            _internalObj = value;
        }
    }

    public uint Uint32
    {
        get => (uint)InternalObj; set
        {
            if (_internalObj != null)
                throw new Exception("Multiple values defined for variant.");
            Type = VariantType.Uint32;
            _internalObj = value;
        }
    }

    public ulong Uint64
    {
        get => (ulong)InternalObj; set
        {
            if (_internalObj != null)
                throw new Exception("Multiple values defined for variant.");
            Type = VariantType.Uint64;
            _internalObj = value;
        }
    }

    public float Float
    {
        get => (float)InternalObj; set
        {
            if (_internalObj != null)
                throw new Exception("Multiple values defined for variant.");
            Type = VariantType.Float;
            _internalObj = value;
        }
    }


    private static float[] CheckFloatLength(float[] inArray, int length) => inArray.Length == length ? inArray : throw new Exception($"Input array length mismatch: got {inArray.Length}, expecting {length}");

    public float[] Vec2
    {
        get => (float[])InternalObj; set
        {
            if (_internalObj != null)
                throw new Exception("Multiple values defined for variant.");
            _internalObj = CheckFloatLength(value, 2);
            Type = VariantType.Vec2;
        }
    }

    public float[] Vec3
    {
        get => (float[])InternalObj; set
        {
            if (_internalObj != null)
                throw new Exception("Multiple values defined for variant.");
            _internalObj = CheckFloatLength(value, 3);
            Type = VariantType.Vec3;
        }
    }

    public float[] Vec4
    {
        get => (float[])InternalObj; set
        {
            if (_internalObj != null)
                throw new Exception("Multiple values defined for variant.");
            _internalObj = CheckFloatLength(value, 4);
            Type = VariantType.Vec4;
        }
    }

    public float[] Mat2
    {
        get => (float[])InternalObj; set
        {
            if (_internalObj != null)
                throw new Exception("Multiple values defined for variant.");
            _internalObj = CheckFloatLength(value, 4);
            Type = VariantType.Mat2;
        }
    }

    public float[] Mat3
    {
        get => (float[])InternalObj; set
        {
            if (_internalObj != null)
                throw new Exception("Multiple values defined for variant.");
            _internalObj = CheckFloatLength(value, 9);
            Type = VariantType.Mat3;
        }
    }

    public float[] Mat4
    {
        get => (float[])InternalObj; set
        {
            if (_internalObj != null)
                throw new Exception("Multiple values defined for variant.");
            _internalObj = CheckFloatLength(value, 16);
            Type = VariantType.Mat4;
        }
    }

    public string Path
    {
        get => (string)InternalObj; set
        {
            if (_internalObj != null)
                throw new Exception("Multiple values defined for variant.");
            _internalObj = value;
            Type = VariantType.Path;
        }
    }

    public string GetString() => InternalObj.ToString() ?? throw new Exception("Unexpected null string.");

    private static void WriteFloatArray(Span<byte> dest, float[] input)
    {
        for (int i = 0; i < input.Length; i++)
        {
            BinaryPrimitives.WriteSingleLittleEndian(dest.Slice(i * 4, 4), input[i]);
        }
    }

    private int GetLength() => Type switch
    {
        VariantType.Byte => 1,
        VariantType.Int32 => 4,
        VariantType.Int64 => 8,
        VariantType.Uint32 => 4,
        VariantType.Uint64 => 8,
        VariantType.Float => 4,
        VariantType.Vec2 => 8,
        VariantType.Vec3 => 12,
        VariantType.Vec4 => 16,
        VariantType.Mat2 => 16,
        VariantType.Mat3 => 36,
        VariantType.Mat4 => 64,
        VariantType.Path => 16,
        _ => throw new Exception("Variant type unset.")
    };

    private void Write(Span<byte> dest)
    {
        switch (Type)
        {
            case VariantType.Byte:
                dest[0] = Byte;
                break;
            case VariantType.Int32:
                BinaryPrimitives.WriteInt32LittleEndian(dest, Int32);
                break;
            case VariantType.Int64:
                BinaryPrimitives.WriteInt64LittleEndian(dest, Int64);
                break;
            case VariantType.Uint32:
                BinaryPrimitives.WriteUInt32LittleEndian(dest, Uint32);
                break;
            case VariantType.Uint64:
                BinaryPrimitives.WriteUInt64LittleEndian(dest, Uint64);
                break;
            case VariantType.Float:
                BinaryPrimitives.WriteSingleLittleEndian(dest, Float);
                break;
            case VariantType.Vec2:
            case VariantType.Vec3:
            case VariantType.Vec4:
            case VariantType.Mat2:
            case VariantType.Mat3:
            case VariantType.Mat4:
                WriteFloatArray(dest, (float[])InternalObj);
                break;
            case VariantType.Path:
                MD5.HashData(Encoding.UTF8.GetBytes(Path), dest);
                break;
            default:
                throw new Exception("Invalid variant type.");
        }
    }

    public byte[] GetBytes()
    {
        byte[] output = new byte[sizeof(byte) + GetLength()];
        output[0] = (byte)Type;
        Write(output.AsSpan(1));
        return output;
    }
}

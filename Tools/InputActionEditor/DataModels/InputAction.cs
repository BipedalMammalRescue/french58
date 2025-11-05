using System.Text;
using System.Text.Json.Serialization;

namespace DataModels;

[JsonPolymorphic]
[JsonDerivedType(typeof(DiscreteInputAction), "discrete")]
[JsonDerivedType(typeof(EmissionInputAction), "emission")]
public abstract class InputAction
{
    public required string Name { get; set; }

    public IEnumerable<byte> Print()
    {
        byte[] nameBuffer = Encoding.Unicode.GetBytes(Name);
        return BitConverter.GetBytes(nameBuffer.Length).Concat(nameBuffer).Concat(PrintCore());
    }
    
    protected abstract IEnumerable<byte> PrintCore();
}

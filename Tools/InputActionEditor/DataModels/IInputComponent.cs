using System.Text.Json.Serialization;

namespace DataModels;

[JsonPolymorphic]
[JsonDerivedType(typeof(KeyboardInputComponent), "keyboard")]
public interface IInputComponent
{
    IEnumerable<byte> Print();
}

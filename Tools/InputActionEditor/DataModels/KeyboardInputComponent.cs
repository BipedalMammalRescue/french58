
namespace DataModels;

public class KeyboardInputComponent : IInputComponent
{
    public required uint ScanCode { get; set; }

    public IEnumerable<byte> Print() => BitConverter.GetBytes(ScanCode).Prepend((byte)InputComponentTypeCode.Keyboard);
}
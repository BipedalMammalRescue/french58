using System.Text;

namespace DataModels;

/// <summary>
/// Discrete input actions interpret the input component as a digital switch.
/// </summary>
public class DiscreteInputAction : InputAction
{
    public required IInputComponent Trigger { get; set; }

    protected override IEnumerable<byte> PrintCore() => Trigger.Print().Prepend((byte)InputActionTypeCode.Discrete);
}

public class EmissionInputAction : InputAction
{
    public required IInputComponent Trigger { get; set; }

    protected override IEnumerable<byte> PrintCore() => Trigger.Print().Prepend((byte)InputActionTypeCode.Emission);
}
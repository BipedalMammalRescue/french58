using MuThr.DataModels.BuildActions;

namespace EntityBuilder.Abstractions;

public class Recipe
{
    public required string Module { get; set; }
    public required string Type { get; set; }
    public required BuildAction Action { get; set; }
}

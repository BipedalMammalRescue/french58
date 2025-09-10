using System;
using System.Text.Json.Nodes;

namespace SignourneyEngine.Companion.DataModels;

public class BuildTask
{
    public required BuildTask[] Children { get; set; }
    public required BuildActionType Type { get; set; }
    public required JsonNode Action { get; set; }
}

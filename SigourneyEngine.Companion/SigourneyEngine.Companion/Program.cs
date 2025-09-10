using System.Text.Json;
using SignourneyEngine.Companion.DataModels.BuildActions;

Console.WriteLine("hello world");

string sample = "{\"$type\": \"command\", \"Process\":\"echo\",\"Arguments\":[\"hello\",\"world\"],\"InputComponents\":[],\"OutputComponents\":[],\"Tags\":{}}";
BuildAction? action = JsonSerializer.Deserialize<BuildAction>(sample);

Console.WriteLine(JsonSerializer.Serialize(action));
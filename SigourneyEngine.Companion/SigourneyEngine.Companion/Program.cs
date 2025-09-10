using System.Collections.Immutable;
using System.Text.Json;
using SignourneyEngine.Companion.DataModels.BuildActions;

Console.WriteLine("hello world");

Dictionary<string, string> source = new()
{
    ["foo"] = "bar",
    ["bar"] = "foo"
};

BuildResult[] children = [
    new BuildResult() { OutputPath = "p1", Tags = ImmutableDictionary<string, string>.Empty.Add("key", "value1")},
    new BuildResult() { OutputPath = "p2", Tags = ImmutableDictionary<string, string>.Empty.Add("key", "value2")}
];

BuildEnvironment env = new(source, children);

Console.WriteLine(env.ExpandValues("#(foo)? maybe #0:key at #1"));
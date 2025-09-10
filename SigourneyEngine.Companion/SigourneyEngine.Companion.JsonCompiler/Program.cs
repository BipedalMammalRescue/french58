string value = "hello %PATH%";

Console.WriteLine(Environment.ExpandEnvironmentVariables(value));
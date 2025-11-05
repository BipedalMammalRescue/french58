using System.Text.Json;
using DataModels;

internal class Program
{
    private static int Main(string[] args)
    {
        if (args.Length != 1)
        {
            Console.Error.WriteLine($"Exactly one command line argument is required, but {args.Length} is given!");
            return 1;
        }

        if (!File.Exists(args[0]))
        {
            Console.Error.WriteLine("Input file does not exist!");
            return 2;
        }

        try
        {
            using FileStream sourceFs = File.OpenRead(args[0]);
            InputAction? action = JsonSerializer.Deserialize<InputAction>(sourceFs);

            if (action == null)
            {
                Console.Error.WriteLine("Input file is null value!");
                return 3;
            }

            using Stream outputStream = Console.OpenStandardOutput();
            outputStream.Write(action.Print().ToArray());
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine("Exception happened:");
            Console.Error.WriteLine(ex);
            return -1;
        }

        return 0;
    }
}
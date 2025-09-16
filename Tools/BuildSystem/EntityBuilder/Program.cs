using System.CommandLine;
using System.CommandLine.Parsing;
using EntityBuilder.Routines;
using MuThr.DataModels.Diagnostic;
using MuThr.Sdk;
using Serilog;
using Serilog.Formatting.Display;
using Serilog.Formatting.Json;

internal class Program
{
    private static async Task<int> Main(string[] args)
    {
        // create the command
        RootCommand rootCommand = new("Entity Builder: a Sigourney Engine companion application.");

        // get logger configs
        Option<DirectoryInfo> logOption = new("--log-path");
        DirectoryInfo? logPath = logOption.Parse(args).GetValueForOption(logOption);

        Option<bool> logAsJsonOption = new("--log-as-json");
        logAsJsonOption.Parse(args);
        bool logAsJson = logAsJsonOption.Parse(args).GetValueForOption(logAsJsonOption);

        // compatibility
        rootCommand.AddGlobalOption(logOption);
        rootCommand.AddGlobalOption(logAsJsonOption);

        // initialize logger
        MessageTemplateTextFormatter verboseFormatter = new("[{Timestamp:HH:mm:ss} {Level:u3}][{PrimaryChannel}] {Message:lj}{NewLine}{Exception}");
        LoggerConfiguration serilogConfig = new LoggerConfiguration()
            .Enrich.With(new MuThrLogEnricher())
            .Enrich.FromLogContext()
            .WriteTo.Console(verboseFormatter);

        serilogConfig = logPath == null ?
            serilogConfig
            : logAsJson ?
                serilogConfig
                    .WriteTo.File(new JsonFormatter(), Path.Combine(logPath.FullName, $"entity_builder_log_{DateTime.Now.ToFileTime()}.log"))
                : serilogConfig
                    .WriteTo.File(verboseFormatter, Path.Combine(logPath.FullName, $"entity_builder_log_{DateTime.Now.ToFileTime()}.log"));

        var logger = new MuThrLogger(["Program"], serilogConfig.CreateLogger());

        // actual commands
        rootCommand.AddCommand(new BuildAssetCommand(logger.WithChannel(nameof(BuildAssetCommand))));

        try
        {
            return await rootCommand.InvokeAsync(args).ConfigureAwait(false);
        }
        catch (Exception ex)
        {
            ((IMuThrLogger)logger).Error(ex, "Unexpected error during execution.");
            return (int)ErrorCodes.UnexpectedError;
        }
    }
}

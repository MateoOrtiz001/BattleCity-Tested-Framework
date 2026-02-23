using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using BattleCity.Orchestrator.Core;
using BattleCity.Orchestrator.Analysis;
using BattleCity.Orchestrator.Cheats;

namespace BattleCity.Orchestrator
{
    internal class Program
    {
        private static bool _verbose = false;
        private static bool _isConsoleRedirected = false;

        static async Task<int> Main(string[] args)
        {
            Console.OutputEncoding = System.Text.Encoding.UTF8;

            // Detectar si la consola está redirigida (pipes, archivos)
            try
            {
                _ = Console.CursorLeft;
                _isConsoleRedirected = false;
            }
            catch
            {
                _isConsoleRedirected = true;
            }

            PrintBanner();

            if (args.Length == 0 || args[0] is "--help" or "-h")
            {
                PrintUsage();
                return 0;
            }

            var command = args[0].ToLowerInvariant();

            try
            {
                return command switch
                {
                    "run" => await RunCommand(args[1..]),
                    "rerun-failed" => await RerunFailedCommand(args[1..]),
                    "build-cheats" => await BuildCheatsCommand(args[1..]),
                    _ => HandleUnknownCommand(command)
                };
            }
            catch (Exception ex)
            {
                WriteColor($"\n  [FATAL] {ex.Message}", ConsoleColor.Red);
                if (_verbose)
                    WriteColor($"  {ex.StackTrace}", ConsoleColor.DarkRed);
                return 99;
            }
        }

        // ── Comando: run ─────────────────────────────────────────────

        private static async Task<int> RunCommand(string[] args)
        {
            var options = ParseRunOptions(args);
            if (options is null)
                return 1;

            if (!File.Exists(options.ExecutablePath))
            {
                WriteColor($"  [ERROR] Executable not found: {options.ExecutablePath}", ConsoleColor.Red);
                return 1;
            }

            PrintRunConfig(options);

            var runnerConfig = new TestRunnerConfig
            {
                ExecutablePath = options.ExecutablePath,
                MaxParallelMatches = options.Parallel,
                OutputDirectory = options.OutputDir,
                ErrorLogDirectory = Path.Combine(options.OutputDir, "errors"),
                RetryOnCrash = options.Retry,
                MaxRetries = options.Retry ? 1 : 0
            };

            var runner = new TestRunner(runnerConfig);

            var progressLock = new object();
            runner.OnProgress += progress =>
            {
                lock (progressLock)
                {
                    PrintProgress(progress);
                }
            };

            using var cts = new CancellationTokenSource();
            Console.CancelKeyPress += (_, e) =>
            {
                e.Cancel = true;
                WriteColor("\n  [WARN] Cancelling... waiting for running matches to finish.", ConsoleColor.Yellow);
                cts.Cancel();
            };

            WriteColor("\n  Starting test run...\n", ConsoleColor.Cyan);

            TestRunReport report;

            try
            {
                var matches = Enumerable.Range(options.StartSeed, options.Count)
                    .Select(seed => new MatchConfig
                    {
                        Seed = seed,
                        MaxFrames = options.MaxFrames,
                        CheatsFilePath = options.CheatsFile,
                        ProcessTimeout = TimeSpan.FromSeconds(options.TimeoutSeconds),
                        MatchId = $"seed_{seed}",
                        ExtraArguments = new Dictionary<string, string>
                        {
                            ["tickRate"] = options.TickRate.ToString(),
                            ["level"] = options.Level,
                            ["teamAPolicy"] = options.TeamAPolicy,
                            ["teamBPolicy"] = options.TeamBPolicy
                        },
                        TankPolicySpecs = new List<string>(options.TankPolicySpecs)
                    })
                    .ToList();

                report = await runner.RunAsync(matches, cts.Token);
            }
            catch (Exception ex)
            {
                Console.WriteLine();
                WriteColor($"  [ERROR] Fatal error: {ex.Message}", ConsoleColor.Red);
                if (_verbose)
                    WriteColor($"    {ex.StackTrace}", ConsoleColor.DarkRed);
                return 2;
            }

            Console.WriteLine();
            Console.WriteLine();

            PrintReport(report);
            
            var stats = new StatsAggregator().Aggregate(report, options.MaxFrames);
            Console.WriteLine($" Anomalies: {stats.AnomaliesCount}");
            if (stats.AnomaliesCount >0 && _verbose)
            {
                foreach(var id in stats.AnomalyMatchIds.Take(20))
                {
                    Console.WriteLine($"  - {id}");
                }
            }

            ExportReports(report, stats, options.OutputDir);

            if (report.FailedCount > 0)
                PrintFailedMatches(report);

            SaveReportSummary(report, options.OutputDir);

            return report.FailedCount > 0 ? 1 : 0;
        }

        // ── Comando: rerun-failed ────────────────────────────────────

        private static async Task<int> RerunFailedCommand(string[] args)
        {
            var options = ParseRunOptions(args);
            if (options is null)
                return 1;

            if (!File.Exists(options.ExecutablePath))
            {
                WriteColor($"  [ERROR] Executable not found: {options.ExecutablePath}", ConsoleColor.Red);
                return 1;
            }

            var errorDir = Path.Combine(options.OutputDir, "errors");

            if (!Directory.Exists(errorDir))
            {
                WriteColor($"  [ERROR] Error directory not found: {errorDir}", ConsoleColor.Red);
                WriteColor("    Run 'run' first to generate results.", ConsoleColor.Gray);
                return 1;
            }

            var errorLogs = Directory.GetFiles(errorDir, "error_seed_*.log");

            if (errorLogs.Length == 0)
            {
                WriteColor("  [OK] No failed matches found. Nothing to rerun.", ConsoleColor.Green);
                return 0;
            }

            var failedSeeds = new List<int>();
            foreach (var logFile in errorLogs)
            {
                var fileName = Path.GetFileNameWithoutExtension(logFile);
                var parts = fileName.Split('_');
                if (parts.Length >= 3 && int.TryParse(parts[2], out int seed))
                {
                    if (!failedSeeds.Contains(seed))
                        failedSeeds.Add(seed);
                }
            }

            WriteColor($"  [INFO] Found {failedSeeds.Count} failed seeds to rerun.", ConsoleColor.Yellow);

            var matches = failedSeeds.Select(seed => new MatchConfig
            {
                Seed = seed,
                MaxFrames = options.MaxFrames,
                CheatsFilePath = options.CheatsFile,
                ProcessTimeout = TimeSpan.FromSeconds(options.TimeoutSeconds),
                MatchId = $"seed_{seed}",
                ExtraArguments = new Dictionary<string, string>
                {
                    ["tickRate"] = options.TickRate.ToString(),
                    ["level"] = options.Level,
                    ["teamAPolicy"] = options.TeamAPolicy,
                    ["teamBPolicy"] = options.TeamBPolicy
                },
                TankPolicySpecs = new List<string>(options.TankPolicySpecs)
            }).ToList();

            var rerunOutputDir = Path.Combine(options.OutputDir, "rerun");

            var runnerConfig = new TestRunnerConfig
            {
                ExecutablePath = options.ExecutablePath,
                MaxParallelMatches = options.Parallel,
                OutputDirectory = rerunOutputDir,
                ErrorLogDirectory = Path.Combine(rerunOutputDir, "errors"),
                RetryOnCrash = options.Retry
            };

            var runner = new TestRunner(runnerConfig);

            var progressLock = new object();
            runner.OnProgress += progress =>
            {
                lock (progressLock)
                {
                    PrintProgress(progress);
                }
            };

            using var cts = new CancellationTokenSource();
            Console.CancelKeyPress += (_, e) => { e.Cancel = true; cts.Cancel(); };

            WriteColor("\n  Rerunning failed matches...\n", ConsoleColor.Cyan);

            var report = await runner.RunAsync(matches, cts.Token);

            Console.WriteLine();
            Console.WriteLine();

            PrintReport(report);

            var stats = new StatsAggregator().Aggregate(report, options.MaxFrames);
            Console.WriteLine($" Anomalies: {stats.AnomaliesCount}");
            if (stats.AnomaliesCount > 0 && _verbose)
            {
                foreach (var id in stats.AnomalyMatchIds.Take(20))
                {
                    Console.WriteLine($"  - {id}");
                }
            }

            ExportReports(report, stats, rerunOutputDir);

            if (report.FailedCount > 0)
                PrintFailedMatches(report);

            SaveReportSummary(report, rerunOutputDir);

            return report.FailedCount > 0 ? 1 : 0;
        }

        // ── Comando: build-cheats ───────────────────────────────────

        private sealed class BuildCheatsOptions
        {
            public string OutputPath { get; set; } = string.Empty;
            public string? Preset { get; set; }
            public bool PrintToConsole { get; set; }
            public List<string> AtCommands { get; } = new();
            public List<string> Comments { get; } = new();

            public int? ManyCount { get; set; }
            public string SpawnAgentType { get; set; } = "attack_base";
            public char DestroyBaseTeam { get; set; } = 'A';
        }

        private static Task<int> BuildCheatsCommand(string[] args)
        {
            var options = ParseBuildCheatsOptions(args);
            if (options is null)
                return Task.FromResult(1);

            var builder = new CheatScriptBuilder()
                .Comment("Generated by BattleCity.Orchestrator");

            try
            {
                ApplyPreset(options.Preset, builder, options.SpawnAgentType);
            }
            catch (ArgumentException ex)
            {
                WriteColor($"  [ERROR] {ex.Message}", ConsoleColor.Red);
                return Task.FromResult(1);
            }

            foreach (var comment in options.Comments) builder.Comment(comment);

            if (options.ManyCount.HasValue)
            {
                var count = options.ManyCount.Value;
                builder.CommandAt(1, $"spawn_tanks {count} A {options.SpawnAgentType}")
                    .CommandAt(1, $"spawn_tanks {count} B {options.SpawnAgentType}")
                    .Comment($"many-count: {count}, agent: {options.SpawnAgentType}");
            }

            if (!string.IsNullOrWhiteSpace(options.Preset) &&
                (options.Preset.Equals("destroy-base", StringComparison.OrdinalIgnoreCase) ||
                 options.Preset.Equals("destroy-base-team", StringComparison.OrdinalIgnoreCase)))
            {
                builder.CommandAt(1, $"destroy_base {options.DestroyBaseTeam}");
            }

            foreach (var entry in options.AtCommands)
            {
                if (!TryParseAtCommand(entry, out var frame, out var command))
                {
                    WriteColor($"  [ERROR] Invalid --at '{entry}'. Use frame:command.", ConsoleColor.Red);
                    return Task.FromResult(1);
                }

                builder.CommandAt(frame, command);
            }

            try
            {
                builder.SaveScheduledScript(options.OutputPath);
                WriteColor($"  [OK] Cheat file generated: {Path.GetFullPath(options.OutputPath)}", ConsoleColor.Green);

                if (options.PrintToConsole)
                {
                    Console.WriteLine();
                    Console.WriteLine(builder.BuildScheduledScript());
                }

                return Task.FromResult(0);
            }
            catch (Exception ex)
            {
                WriteColor($"  [ERROR] Failed to generate cheat script: {ex.Message}", ConsoleColor.Red);
                return Task.FromResult(1);
            }
        }

        private static BuildCheatsOptions? ParseBuildCheatsOptions(string[] args)
        {
            var options = new BuildCheatsOptions();

            try
            {
                for (int i = 0; i < args.Length; i++)
                {
                    switch (args[i].ToLowerInvariant())
                    {
                        case "--out":
                            options.OutputPath = GetNextArg(args, ref i, "--out");
                            break;
                        case "--preset":
                            options.Preset = GetNextArg(args, ref i, "--preset");
                            break;
                        case "--at":
                            options.AtCommands.Add(GetNextArg(args, ref i, "--at"));
                            break;
                        case "--comment":
                            options.Comments.Add(GetNextArg(args, ref i, "--comment"));
                            break;
                        case "--print":
                            options.PrintToConsole = true;
                            break;
                        case "--many-count":
                            options.ManyCount = ParseInt(GetNextArg(args, ref i, "--many-count"), "--many-count");
                            break;
                        case "--spawn-agent":
                            options.SpawnAgentType = GetNextArg(args, ref i, "--spawn-agent");
                            break;
                        case "--destroy-base-team":
                            var team = GetNextArg(args, ref i, "--destroy-base-team").Trim().ToUpperInvariant();
                            if (team != "A" && team != "B")
                                throw new ArgumentException("--destroy-base-team must be A or B.");
                            options.DestroyBaseTeam = team[0];
                            break;
                        default:
                            WriteColor($"  [ERROR] Unknown option for build-cheats: {args[i]}", ConsoleColor.Red);
                            return null;
                    }
                }
            }
            catch (ArgumentException ex)
            {
                WriteColor($"  [ERROR] {ex.Message}", ConsoleColor.Red);
                return null;
            }

            if (string.IsNullOrWhiteSpace(options.OutputPath))
            {
                WriteColor("  [ERROR] --out is required for build-cheats.", ConsoleColor.Red);
                return null;
            }

            if (options.ManyCount.HasValue && options.ManyCount.Value <= 0)
            {
                WriteColor("  [ERROR] --many-count must be > 0.", ConsoleColor.Red);
                return null;
            }

            return options;
        }

        private static void ApplyPreset(string? preset, CheatScriptBuilder builder, string spawnAgentType)
        {
            if (string.IsNullOrWhiteSpace(preset))
                return;

            switch (preset.Trim().ToLowerInvariant())
            {
                case "empty-map":
                    builder.CommandAt(1, "clear_walls")
                        .CommandAt(1, "remove_all_tanks A")
                        .CommandAt(1, "remove_all_tanks B")
                        .Comment("Preset: empty-map");
                    break;
                case "1v1":
                    builder.CommandAt(1, "remove_all_tanks A")
                        .CommandAt(1, "remove_all_tanks B")
                        .CommandAt(2, $"spawn_tanks 1 A {spawnAgentType}")
                        .CommandAt(2, $"spawn_tanks 1 B {spawnAgentType}")
                        .Comment("Preset: 1v1");
                    break;
                case "many-vs-many":
                    builder.CommandAt(1, $"spawn_tanks 10 A {spawnAgentType}")
                        .CommandAt(1, $"spawn_tanks 10 B {spawnAgentType}")
                        .Comment("Preset: many-vs-many");
                    break;
                case "destroy-base-a":
                    builder.CommandAt(1, "destroy_base A")
                        .Comment("Preset: destroy-base-a");
                    break;
                case "destroy-base-b":
                    builder.CommandAt(1, "destroy_base B")
                        .Comment("Preset: destroy-base-b");
                    break;
                case "destroy-base":
                    builder.Comment("Preset: destroy-base (team selected via --destroy-base-team)");
                    break;
                case "stress":
                    builder.CommandAt(1, $"spawn_tanks 50 A {spawnAgentType}")
                        .CommandAt(1, $"spawn_tanks 50 B {spawnAgentType}")
                        .Comment("Preset: stress");
                    break;
                default:
                    throw new ArgumentException($"Unknown preset '{preset}'. Allowed: empty-map, 1v1, many-vs-many, destroy-base, destroy-base-a, destroy-base-b, stress.");
            }
        }

        private static bool TryParseAtCommand(string input, out int frame, out string command)
        {
            frame = 0;
            command = string.Empty;

            if (string.IsNullOrWhiteSpace(input))
                return false;

            var idx = input.IndexOf(':');
            if (idx <= 0 || idx >= input.Length - 1)
                return false;

            var framePart = input.Substring(0, idx).Trim();
            command = input[(idx + 1)..].Trim();

            return int.TryParse(framePart, out frame) && frame >= 0 && !string.IsNullOrWhiteSpace(command);
        }

        // ── Parseo de argumentos ─────────────────────────────────────

        private class RunOptions
        {
            public string ExecutablePath { get; set; } = string.Empty;
            public int Count { get; set; } = 10;
            public int StartSeed { get; set; } = 0;
            public int MaxFrames { get; set; } = 10_000;
            public int TickRate { get; set; } = 10;
            public string Level { get; set; } = "level1";
            public int Parallel { get; set; } = Environment.ProcessorCount;
            public int TimeoutSeconds { get; set; } = 60;
            public string? CheatsFile { get; set; }
            public string OutputDir { get; set; } = "results";
            public bool Retry { get; set; } = false;
            public string TeamAPolicy { get; set; } = "attack_base";
            public string TeamBPolicy { get; set; } = "attack_base";
            public List<string> TankPolicySpecs { get; } = new();
        }

        private static RunOptions? ParseRunOptions(string[] args)
        {
            var options = new RunOptions();

            try
            {
                for (int i = 0; i < args.Length; i++)
                {
                    switch (args[i].ToLowerInvariant())
                    {
                        case "--exe":
                            options.ExecutablePath = GetNextArg(args, ref i, "--exe");
                            break;
                        case "--count":
                            options.Count = ParseInt(GetNextArg(args, ref i, "--count"), "--count");
                            break;
                        case "--start-seed":
                            options.StartSeed = ParseInt(GetNextArg(args, ref i, "--start-seed"), "--start-seed");
                            break;
                        case "--max-frames":
                            options.MaxFrames = ParseInt(GetNextArg(args, ref i, "--max-frames"), "--max-frames");
                            break;
                        case "--tick-rate":
                            options.TickRate = ParseInt(GetNextArg(args, ref i, "--tick-rate"), "--tick-rate");
                            break;
                        case "--level":
                            options.Level = GetNextArg(args, ref i, "--level");
                            break;
                        case "--parallel":
                            options.Parallel = ParseInt(GetNextArg(args, ref i, "--parallel"), "--parallel");
                            break;
                        case "--timeout":
                            options.TimeoutSeconds = ParseInt(GetNextArg(args, ref i, "--timeout"), "--timeout");
                            break;
                        case "--cheats":
                            options.CheatsFile = GetNextArg(args, ref i, "--cheats");
                            break;
                        case "--output-dir":
                            options.OutputDir = GetNextArg(args, ref i, "--output-dir");
                            break;
                        case "--team-a-policy":
                            options.TeamAPolicy = GetNextArg(args, ref i, "--team-a-policy");
                            break;
                        case "--team-b-policy":
                            options.TeamBPolicy = GetNextArg(args, ref i, "--team-b-policy");
                            break;
                        case "--tank-policy":
                            options.TankPolicySpecs.Add(GetNextArg(args, ref i, "--tank-policy"));
                            break;
                        case "--retry":
                            options.Retry = true;
                            break;
                        case "--verbose":
                            _verbose = true;
                            break;
                        default:
                            WriteColor($"  [ERROR] Unknown option: {args[i]}", ConsoleColor.Red);
                            PrintUsage();
                            return null;
                    }
                }
            }
            catch (ArgumentException ex)
            {
                WriteColor($"  [ERROR] {ex.Message}", ConsoleColor.Red);
                return null;
            }

            if (string.IsNullOrEmpty(options.ExecutablePath))
            {
                WriteColor("  [ERROR] --exe is required.", ConsoleColor.Red);
                PrintUsage();
                return null;
            }

            // Validaciones de rango
            if (options.Count <= 0)
            {
                WriteColor("  [ERROR] --count must be > 0.", ConsoleColor.Red);
                return null;
            }

            if (options.Parallel <= 0)
            {
                WriteColor("  [ERROR] --parallel must be > 0.", ConsoleColor.Red);
                return null;
            }

            if (options.MaxFrames <= 0)
            {
                WriteColor("  [ERROR] --max-frames must be > 0.", ConsoleColor.Red);
                return null;
            }

            if (options.TickRate <= 0)
            {
                WriteColor("  [ERROR] --tick-rate must be > 0.", ConsoleColor.Red);
                return null;
            }

            if (string.IsNullOrWhiteSpace(options.Level))
            {
                WriteColor("  [ERROR] --level cannot be empty.", ConsoleColor.Red);
                return null;
            }

            if (options.TimeoutSeconds <= 0)
            {
                WriteColor("  [ERROR] --timeout must be > 0.", ConsoleColor.Red);
                return null;
            }

            // Validar archivo de cheats si se proporcionó
            if (options.CheatsFile is not null && !File.Exists(options.CheatsFile))
            {
                WriteColor($"  [ERROR] Cheats file not found: {options.CheatsFile}", ConsoleColor.Red);
                return null;
            }

            options.ExecutablePath = Path.GetFullPath(options.ExecutablePath);
            options.OutputDir = Path.GetFullPath(options.OutputDir);
            if (options.CheatsFile is not null)
            {
                options.CheatsFile = Path.GetFullPath(options.CheatsFile);
            }

            return options;
        }

        private static string GetNextArg(string[] args, ref int index, string optionName)
        {
            index++;
            if (index >= args.Length)
                throw new ArgumentException($"Option {optionName} requires a value.");
            return args[index];
        }

        private static int ParseInt(string value, string optionName)
        {
            if (!int.TryParse(value, out int result))
                throw new ArgumentException($"Option {optionName} requires an integer value, got: '{value}'");
            return result;
        }

        // ── Impresión ────────────────────────────────────────────────

        private static void PrintBanner()
        {
            WriteColor(@"
  ========================================
       BattleCity Test Orchestrator       
  ========================================
", ConsoleColor.Magenta);
        }

        private static void PrintUsage()
        {
            Console.WriteLine(@"
  Usage:
    BattleCity.Orchestrator run [options]
    BattleCity.Orchestrator rerun-failed [options]
    BattleCity.Orchestrator build-cheats [options]

  Options:
    --exe <path>           Path to BattleCityHeadless executable (required)
    --count <N>            Number of matches to run (default: 10)
    --start-seed <N>       Starting seed (default: 0)
    --max-frames <N>       Max frames per match (default: 10000)
    --tick-rate <N>        Headless tick rate (default: 10)
    --level <name>         level1..level5 (default: level1)
    --parallel <N>         Max parallel matches (default: CPU cores)
    --timeout <seconds>    Timeout per match in seconds (default: 60)
    --cheats <path>        Path to cheats file
    --team-a-policy <type> Team A default agent (attack_base|random|defensive|astar_attack|interceptor)
    --team-b-policy <type> Team B default agent (attack_base|random|defensive|astar_attack|interceptor)
    --tank-policy <id:type> Repeatable per-tank agent override
    --output-dir <path>    Output directory (default: results)
    --retry                Retry crashed matches once
    --verbose              Show detailed output

    build-cheats options:
        --out <path>                  Output file (required)
        --preset <empty-map|1v1|many-vs-many|destroy-base|destroy-base-a|destroy-base-b|stress>
        --at <frame:command>          Repeatable (Runner format)
        --comment <text>              Repeatable
        --many-count <N>              Adds spawn_tanks N A/B <spawn-agent> at frame 1
        --spawn-agent <type>          Agent type for generated spawn_tanks commands (default: attack_base)
        --destroy-base-team <A|B>     For destroy-base preset variant
        --print

  Examples:
    BattleCity.Orchestrator run --exe game.exe --count 100
    BattleCity.Orchestrator run --exe game.exe --count 50 --parallel 8 --retry
        BattleCity.Orchestrator run --exe game.exe --team-a-policy astar_attack --team-b-policy defensive --tank-policy ""2:random""
    BattleCity.Orchestrator rerun-failed --exe game.exe --output-dir results
    BattleCity.Orchestrator build-cheats --out cheats\stress.txt --preset stress --print
        BattleCity.Orchestrator build-cheats --out cheats\custom.txt --at ""10:spawn_tanks 3 A attack_base"" --at ""20:heal_all A 5""
");
        }

        private static void PrintRunConfig(RunOptions options)
        {
            WriteColor("  Configuration:", ConsoleColor.Cyan);
            Console.WriteLine($"    Executable:     {options.ExecutablePath}");
            Console.WriteLine($"    Matches:        {options.Count}");
            Console.WriteLine($"    Seed range:     {options.StartSeed} -> {options.StartSeed + options.Count - 1}");
            Console.WriteLine($"    Max frames:     {options.MaxFrames}");
            Console.WriteLine($"    Tick rate:      {options.TickRate}");
            Console.WriteLine($"    Level:          {options.Level}");
            Console.WriteLine($"    Parallel:       {options.Parallel}");
            Console.WriteLine($"    Timeout:        {options.TimeoutSeconds}s");
            Console.WriteLine($"    Cheats:         {options.CheatsFile ?? "(none)"}");
            Console.WriteLine($"    Team A policy:  {options.TeamAPolicy}");
            Console.WriteLine($"    Team B policy:  {options.TeamBPolicy}");
            Console.WriteLine($"    Tank policies:  {(options.TankPolicySpecs.Count == 0 ? "(none)" : string.Join(", ", options.TankPolicySpecs))}");
            Console.WriteLine($"    Output dir:     {options.OutputDir}");
            Console.WriteLine($"    Retry on crash: {(options.Retry ? "Yes" : "No")}");
        }

        private static void PrintProgress(TestRunProgress progress)
        {
            var barWidth = 30;
            var filled = (int)(progress.PercentComplete / 100 * barWidth);
            var empty = barWidth - filled;
            var bar = new string('#', filled) + new string('-', empty);

            var statusChar = progress.LastMatchStatus switch
            {
                MatchFinalStatus.CompletedNormally => "+",
                MatchFinalStatus.Draw => "=",
                MatchFinalStatus.CompletedByTimeout => "~",
                MatchFinalStatus.Crashed => "X",
                MatchFinalStatus.TimedOut => "T",
                _ => "?"
            };

            var line = $"  [{bar}] {progress.CompletedMatches}/{progress.TotalMatches} " +
                       $"({progress.PercentComplete:F0}%) " +
                       $"{(progress.FailedMatches > 0 ? $"ERR:{progress.FailedMatches} " : "")}" +
                       $"[{statusChar}] {progress.LastMatchId} " +
                       $"[{progress.Elapsed:mm\\:ss}]";

            if (_isConsoleRedirected)
            {
                // Si está redirigido, imprimir cada línea
                Console.WriteLine(line);
            }
            else
            {
                // Sobreescribir la línea actual
                try
                {
                    Console.SetCursorPosition(0, Console.CursorTop);
                    // Limpiar la línea con espacios
                    Console.Write(new string(' ', Console.WindowWidth - 1));
                    Console.SetCursorPosition(0, Console.CursorTop);
                    Console.Write(line);
                }
                catch
                {
                    // Fallback si hay error con la consola
                    Console.WriteLine(line);
                    _isConsoleRedirected = true;
                }
            }
        }

        private static void PrintReport(TestRunReport report)
        {
            report.PrintSummary(Console.WriteLine);

            Console.WriteLine();

            if (report.WinRateByPlayer.Count > 0)
            {
                WriteColor("  Win Rates:", ConsoleColor.Cyan);
                foreach (var kvp in report.WinRateByPlayer.OrderByDescending(x => x.Value.WinRate))
                {
                    var barLen = (int)(kvp.Value.WinRate / 100 * 20);
                    var bar = new string('#', barLen) + new string('-', 20 - barLen);

                    var color = kvp.Value.WinRate > 50 ? ConsoleColor.Green : ConsoleColor.Yellow;
                    WriteColor($"    {kvp.Key,-12} [{bar}] {kvp.Value.WinRate:F1}% " +
                               $"({kvp.Value.Wins}/{kvp.Value.TotalMatches})", color);
                }
                Console.WriteLine();
            }
        }

        private static void PrintFailedMatches(TestRunReport report)
        {
            WriteColor($"  Failed Matches ({report.FailedCount}):", ConsoleColor.Red);

            var maxToShow = 20;
            foreach (var failed in report.Failed.Take(maxToShow))
            {
                var duration = failed.ProcessResult?.WallClockDuration.TotalSeconds ?? 0;
                Console.WriteLine(
                    $"    {failed.MatchId,-16} " +
                    $"Status: {failed.FinalStatus,-20} " +
                    $"Exit: {failed.ProcessResult?.ExitCode ?? -1,4} " +
                    $"Time: {duration:F1}s");

                if (_verbose && failed.ProcessResult is not null)
                {
                    var stderr = failed.ProcessResult.StandardError;
                    if (!string.IsNullOrWhiteSpace(stderr))
                    {
                        var lines = stderr.Split('\n').Take(3);
                        foreach (var errorLine in lines)
                        {
                            WriteColor($"      | {errorLine.TrimEnd()}", ConsoleColor.DarkRed);
                        }
                    }
                }
            }

            if (report.FailedCount > maxToShow)
            {
                WriteColor($"    ... and {report.FailedCount - maxToShow} more.", ConsoleColor.Yellow);
            }

            Console.WriteLine();
            WriteColor($"  Error logs saved to: {report.Config.ErrorLogDirectory}", ConsoleColor.Cyan);
            Console.WriteLine();
        }

        private static void SaveReportSummary(TestRunReport report, string outputDir)
        {
            try
            {
                Directory.CreateDirectory(outputDir);
                var summaryPath = Path.Combine(outputDir, "summary.txt");

                using var writer = new StreamWriter(summaryPath);

                writer.WriteLine("BattleCity Test Run Summary");
                writer.WriteLine($"Generated: {DateTime.Now:yyyy-MM-dd HH:mm:ss}");
                writer.WriteLine("========================================");
                writer.WriteLine($"Total matches:  {report.TotalMatches}");
                writer.WriteLine($"Completed:      {report.CompletedCount} ({report.SuccessRate:F1}%)");
                writer.WriteLine($"Failed:         {report.FailedCount}");
                writer.WriteLine($"Duration:       {report.TotalDuration:hh\\:mm\\:ss}");
                writer.WriteLine($"Avg match time: {report.AverageMatchDuration:s\\.fff}s");
                writer.WriteLine($"Cancelled:      {report.WasCancelled}");
                writer.WriteLine();

                writer.WriteLine("Status Breakdown:");
                foreach (var kvp in report.StatusBreakdown.OrderByDescending(x => x.Value))
                {
                    writer.WriteLine($"  {kvp.Key,-22} {kvp.Value}");
                }

                writer.WriteLine();
                writer.WriteLine("Win Rates:");
                foreach (var kvp in report.WinRateByPlayer)
                {
                    writer.WriteLine($"  {kvp.Key,-12} {kvp.Value.Wins}/{kvp.Value.TotalMatches} ({kvp.Value.WinRate:F1}%)");
                }

                if (report.FailedCount > 0)
                {
                    writer.WriteLine();
                    writer.WriteLine("Failed Matches:");
                    foreach (var failed in report.Failed)
                    {
                        writer.WriteLine($"  {failed.MatchId}: {failed.FinalStatus} (exit {failed.ProcessResult?.ExitCode})");
                    }
                }

                WriteColor($"  Summary saved to: {summaryPath}", ConsoleColor.Cyan);
            }
            catch (Exception ex)
            {
                WriteColor($"  [WARN] Could not save summary: {ex.Message}", ConsoleColor.Yellow);
            }
        }

        private static void ExportReports(TestRunReport report, AggregatedStats stats, string outputDir)
        {
            try
            {
                var exporter = new ReportExporter();
                exporter.ExportAll(report, stats, outputDir);

                WriteColor($"  Matches CSV: {Path.Combine(outputDir, "matches.csv")}", ConsoleColor.Cyan);
                WriteColor($"  Summary CSV: {Path.Combine(outputDir, "summary.csv")}", ConsoleColor.Cyan);
                WriteColor($"  HTML report: {Path.Combine(outputDir, "report.html")}", ConsoleColor.Cyan);
            }
            catch (Exception ex)
            {
                WriteColor($"  [WARN] Could not export CSV/HTML report: {ex.Message}", ConsoleColor.Yellow);
            }
        }

        

        // ── Helpers ──────────────────────────────────────────────────

        private static void WriteColor(string text, ConsoleColor color)
        {
            var prev = Console.ForegroundColor;
            Console.ForegroundColor = color;
            Console.WriteLine(text);
            Console.ForegroundColor = prev;
        }

        private static int HandleUnknownCommand(string command)
        {
            WriteColor($"  [ERROR] Unknown command: {command}", ConsoleColor.Red);
            PrintUsage();
            return 1;
        }
    }
}
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace BattleCity.Orchestrator.Core
{
    public class TestRunnerConfig
    {
        public string ExecutablePath { get; set; } = string.Empty;
        public int MaxParallelMatches { get; set; } = Environment.ProcessorCount;
        public string OutputDirectory { get; set; } = "results";
        public string ErrorLogDirectory { get; set; } = "errors";
        public bool RetryOnCrash { get; set; } = false;
        public int MaxRetries { get; set; } = 1;
    }

    public class TestRunProgress
    {
        public int TotalMatches { get; init; }
        public int CompletedMatches { get; init; }
        public int FailedMatches { get; init; }
        public string LastMatchId { get; init; } = string.Empty;
        public MatchFinalStatus LastMatchStatus { get; init; }
        public TimeSpan Elapsed { get; init; }

        public double PercentComplete => TotalMatches == 0
            ? 0
            : (double)CompletedMatches / TotalMatches * 100;

        public override string ToString() =>
            $"[{CompletedMatches}/{TotalMatches}] ({PercentComplete:F1}%) " +
            $"Failed: {FailedMatches} | Last: {LastMatchId} -> {LastMatchStatus} | " +
            $"Elapsed: {Elapsed:mm\\:ss}";
    }

    public class TestRunReport
    {
        public DateTime StartedAt { get; init; }
        public DateTime FinishedAt { get; init; }
        public TimeSpan TotalDuration => FinishedAt - StartedAt;

        public List<MatchResult> AllResults { get; init; } = new();
        public TestRunnerConfig Config { get; init; } = new();
        public bool WasCancelled { get; init; }

        public int TotalMatches => AllResults.Count;

        public List<MatchResult> Completed =>
            AllResults.Where(r => !r.IsFailure).ToList();

        public List<MatchResult> Failed =>
            AllResults.Where(r => r.IsFailure).ToList();

        public int CompletedCount => Completed.Count;
        public int FailedCount => Failed.Count;

        public double SuccessRate => TotalMatches == 0
            ? 0
            : (double)CompletedCount / TotalMatches * 100;

        public Dictionary<MatchFinalStatus, int> StatusBreakdown =>
            AllResults
                .GroupBy(r => r.FinalStatus)
                .ToDictionary(g => g.Key, g => g.Count());

        public TimeSpan AverageMatchDuration
        {
            get
            {
                var completedWithProcess = Completed
                    .Where(r => r.ProcessResult is not null)
                    .ToList();

                if (completedWithProcess.Count == 0)
                    return TimeSpan.Zero;

                var totalMs = completedWithProcess
                    .Sum(r => r.ProcessResult!.WallClockDuration.TotalMilliseconds);

                return TimeSpan.FromMilliseconds(totalMs / completedWithProcess.Count);
            }
        }

        public Dictionary<string, WinRateInfo> WinRateByPlayer
        {
            get
            {
                var withWinner = Completed
                    .Where(r => !string.IsNullOrEmpty(r.Winner))
                    .ToList();

                if (withWinner.Count == 0)
                    return new Dictionary<string, WinRateInfo>();

                var allPlayerIds = withWinner
                    .SelectMany(r => r.Players.Select(p => p.Id))
                    .Concat(withWinner.Select(r => r.Winner!).Where(w => !string.IsNullOrWhiteSpace(w) && !string.Equals(w, "Draw", StringComparison.OrdinalIgnoreCase)))
                    .Distinct()
                    .ToList();

                var result = new Dictionary<string, WinRateInfo>();

                foreach (var playerId in allPlayerIds)
                {
                    var wins = withWinner
                        .Count(r => string.Equals(r.Winner, playerId, StringComparison.OrdinalIgnoreCase));

                    result[playerId] = new WinRateInfo
                    {
                        PlayerId = playerId,
                        Wins = wins,
                        TotalMatches = withWinner.Count,
                        WinRate = (double)wins / withWinner.Count * 100
                    };
                }

                return result;
            }
        }

        public void PrintSummary(Action<string> writeLine)
        {
            writeLine("========================================");
            writeLine("         TEST RUN REPORT                ");
            writeLine("========================================");
            writeLine($"  Total matches:    {TotalMatches,5}");
            writeLine($"  Completed:        {CompletedCount,5}  ({SuccessRate:F1}%)");
            writeLine($"  Failed:           {FailedCount,5}");
            writeLine($"  Total duration:   {TotalDuration:hh\\:mm\\:ss}");
            writeLine($"  Avg match time:   {AverageMatchDuration:s\\.fff}s");
            writeLine($"  Cancelled:        {(WasCancelled ? "Yes" : "No")}");
            writeLine("----------------------------------------");

            foreach (var kvp in StatusBreakdown.OrderByDescending(x => x.Value))
            {
                writeLine($"  {kvp.Key,-22} {kvp.Value,5}");
            }

            writeLine("----------------------------------------");

            foreach (var kvp in WinRateByPlayer)
            {
                writeLine($"  {kvp.Key,-12} Wins: {kvp.Value.Wins,3}  Rate: {kvp.Value.WinRate,5:F1}%");
            }

            writeLine("========================================");
        }
    }

    public class WinRateInfo
    {
        public string PlayerId { get; init; } = string.Empty;
        public int Wins { get; init; }
        public int TotalMatches { get; init; }
        public double WinRate { get; init; }
    }

    public class TestRunner
    {
        private readonly TestRunnerConfig _config;
        private readonly MatchProcess _matchProcess;

        public event Action<TestRunProgress>? OnProgress;

        public TestRunner(TestRunnerConfig config)
        {
            _config = config ?? throw new ArgumentNullException(nameof(config));
            _matchProcess = new MatchProcess(config.ExecutablePath);

            Directory.CreateDirectory(config.OutputDirectory);
            Directory.CreateDirectory(config.ErrorLogDirectory);
        }

        public async Task<TestRunReport> RunAsync(
            IReadOnlyList<MatchConfig> matches,
            CancellationToken cancellationToken = default)
        {
            var startTime = DateTime.Now;
            var stopwatch = Stopwatch.StartNew();
            var results = new ConcurrentBag<MatchResult>();

            int completedCount = 0;
            int failedCount = 0;

            using var semaphore = new SemaphoreSlim(_config.MaxParallelMatches);

            var preparedMatches = PrepareMatchConfigs(matches);

            var tasks = preparedMatches.Select(config => Task.Run(async () =>
            {
                await semaphore.WaitAsync(cancellationToken);
                try
                {
                    var result = await ExecuteSingleMatch(config, cancellationToken);

                    if (result.IsFailure && _config.RetryOnCrash)
                    {
                        result = await RetryMatch(config, result, cancellationToken);
                    }

                    results.Add(result);

                    var completed = Interlocked.Increment(ref completedCount);
                    if (result.IsFailure)
                        Interlocked.Increment(ref failedCount);

                    if (result.IsFailure)
                        SaveErrorLog(result);

                    OnProgress?.Invoke(new TestRunProgress
                    {
                        TotalMatches = matches.Count,
                        CompletedMatches = completed,
                        FailedMatches = failedCount,
                        LastMatchId = result.MatchId,
                        LastMatchStatus = result.FinalStatus,
                        Elapsed = stopwatch.Elapsed
                    });
                }
                finally
                {
                    semaphore.Release();
                }
            }, cancellationToken)).ToArray();

            bool wasCancelled = false;
            try
            {
                await Task.WhenAll(tasks);
            }
            catch (OperationCanceledException)
            {
                wasCancelled = true;
            }

            stopwatch.Stop();

            return new TestRunReport
            {
                StartedAt = startTime,
                FinishedAt = DateTime.Now,
                AllResults = results.OrderBy(r => r.MatchId).ToList(),
                Config = _config,
                WasCancelled = wasCancelled
            };
        }

        public async Task<TestRunReport> RunSeedRangeAsync(
            int startSeed,
            int count,
            int maxFrames = 10_000,
            string? cheatsFilePath = null,
            int timeoutSeconds = 60,
            CancellationToken cancellationToken = default)
        {
            var matches = Enumerable.Range(startSeed, count)
                .Select(seed => new MatchConfig
                {
                    Seed = seed,
                    MaxFrames = maxFrames,
                    CheatsFilePath = cheatsFilePath,
                    ProcessTimeout = TimeSpan.FromSeconds(timeoutSeconds),
                    MatchId = $"seed_{seed}"
                })
                .ToList();

            return await RunAsync(matches, cancellationToken);
        }

        private List<MatchConfig> PrepareMatchConfigs(IReadOnlyList<MatchConfig> matches)
        {
            return matches.Select(config =>
            {
                var prepared = config.Clone();

                if (string.IsNullOrEmpty(prepared.OutputJsonPath))
                {
                    prepared.OutputJsonPath = Path.Combine(
                        Path.GetFullPath(_config.OutputDirectory),
                        $"result_{prepared.MatchId}.json");
                }

                return prepared;
            }).ToList();
        }

        private async Task<MatchResult> ExecuteSingleMatch(
            MatchConfig config,
            CancellationToken cancellationToken)
        {
            try
            {
                var processResult = await _matchProcess.RunAsync(config, cancellationToken);
                return MatchResultFactory.FromProcessResult(processResult);
            }
            catch (Exception ex)
            {
                var fakeProcess = new MatchProcessResult
                {
                    MatchId = config.MatchId,
                    Outcome = MatchOutcome.LaunchFailed,
                    ExitCode = -1,
                    WallClockDuration = TimeSpan.Zero,
                    StandardError = $"Orchestrator exception: {ex.Message}",
                    OutputJsonPath = config.OutputJsonPath
                };

                return MatchResultFactory.Failed(fakeProcess, ex.Message);
            }
        }

        private async Task<MatchResult> RetryMatch(
            MatchConfig config,
            MatchResult lastResult,
            CancellationToken cancellationToken)
        {
            for (int attempt = 1; attempt <= _config.MaxRetries; attempt++)
            {
                if (lastResult.FinalStatus != MatchFinalStatus.Crashed)
                    break;

                var retryConfig = config.Clone();
                retryConfig.MatchId = $"{config.MatchId}_retry{attempt}";
                retryConfig.OutputJsonPath = Path.Combine(
                    Path.GetFullPath(_config.OutputDirectory),
                    $"result_{retryConfig.MatchId}.json");

                lastResult = await ExecuteSingleMatch(retryConfig, cancellationToken);

                if (!lastResult.IsFailure)
                    break;
            }

            return lastResult;
        }

        private void SaveErrorLog(MatchResult result)
        {
            if (result.ProcessResult is null)
                return;

            try
            {
                var logPath = Path.Combine(
                    _config.ErrorLogDirectory,
                    $"error_{result.MatchId}.log");

                var pr = result.ProcessResult;
                var content =
                    $"Match ID:    {result.MatchId}\n" +
                    $"Status:      {result.FinalStatus}\n" +
                    $"Exit Code:   {pr.ExitCode}\n" +
                    $"Duration:    {pr.WallClockDuration}\n" +
                    $"End Reason:  {result.EndReason}\n" +
                    $"\n=== STDOUT ===\n{pr.StandardOutput}" +
                    $"\n=== STDERR ===\n{pr.StandardError}";

                File.WriteAllText(logPath, content);
            }
            catch
            {
                // No fallar el runner por no poder escribir un log
            }
        }
    }
}
using System;
using System.Collections.Generic;
using System.Linq;
using BattleCity.Orchestrator.Core;

namespace BattleCity.Orchestrator.Analysis
{
    public sealed class AggregatedStats
    {
        public int TotalMatches{get;init;}
        public int CompletedMatches{get;init;}
        public int FailedMatches{get;init;}
        public double SuccessRate{get;init;}

        public double AverageFrames{get;init;}
        public double AverageDurationMs{get;init;}
        public double AverageScore{get;init;}

        public Dictionary<string, double> WinRateByPlayer{get; init;} = new(StringComparer.OrdinalIgnoreCase);
        public Dictionary<MatchFinalStatus, int> StatusBreakdown {get;init;} = new();

        public int AnomaliesCount{get;init;}
        public List<string> AnomalyMatchIds{get;init;} = new();
    }

    public sealed class StatsAggregator
    {
        public AggregatedStats Aggregate(TestRunReport report, int expectedMaxFrames)
        {
            if (report is null) throw new ArgumentNullException(nameof(report));

            var completed = report.AllResults.Where(r => !r.IsFailure).ToList();
            var failed = report.AllResults.Where(r => r.IsFailure).ToList();

            var avgFrames = completed.Count == 0 ? 0 : completed.Average(r => r.TotalFrames);
            var avgDurationMs = completed.Count == 0 ? 0 : completed.Average(r => r.DurationMs);
            var avgScore = completed.Count == 0 ? 0 : completed.Average(r => r.Score);

            var statusBreakdown = report.AllResults
                .GroupBy(r => r.FinalStatus)
                .ToDictionary(g => g.Key, g => g.Count());

            // Win rate sobre partidas completadas con ganador definido
            var withWinner = completed.Where(r => !string.IsNullOrWhiteSpace(r.Winner)).ToList();
            var winsByPlayer = withWinner
                .Where(r => !string.Equals(r.Winner, "Draw", StringComparison.OrdinalIgnoreCase))
                .GroupBy(r => r.Winner!, StringComparer.OrdinalIgnoreCase)
                .ToDictionary(g => g.Key, g => g.Count(), StringComparer.OrdinalIgnoreCase);

            var winRateByPlayer = new Dictionary<string, double>(StringComparer.OrdinalIgnoreCase);
            foreach (var kv in winsByPlayer)
            {
                winRateByPlayer[kv.Key] = withWinner.Count == 0
                    ? 0
                    : (double)kv.Value / withWinner.Count * 100.0;
            }

            // Anomalías mínimas compatibles con tu modelo actual
            var anomalies = report.AllResults
                .Where(r =>
                    r.TotalFrames >= expectedMaxFrames ||                   // Se fue al límite
                    r.EndReasonParsed == GameEndReason.MaxFramesReached || // Timeout de gameplay
                    r.Score == 0 ||                                         // Sin score
                    (r.DurationMs < 0))                                    // Dato inválido
                .Select(r => r.MatchId)
                .Distinct()
                .ToList();

            return new AggregatedStats
            {
                TotalMatches = report.AllResults.Count,
                CompletedMatches = completed.Count,
                FailedMatches = failed.Count,
                SuccessRate = report.AllResults.Count == 0 ? 0 : (double)completed.Count / report.AllResults.Count * 100.0,
                AverageFrames = avgFrames,
                AverageDurationMs = avgDurationMs,
                AverageScore = avgScore,
                WinRateByPlayer = winRateByPlayer,
                StatusBreakdown = statusBreakdown,
                AnomaliesCount = anomalies.Count,
                AnomalyMatchIds = anomalies
            };
        }
    }
}
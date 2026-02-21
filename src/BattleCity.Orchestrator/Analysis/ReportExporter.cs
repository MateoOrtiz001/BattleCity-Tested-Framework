using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using BattleCity.Orchestrator.Core;

namespace BattleCity.Orchestrator.Analysis
{
    public sealed class ReportExporter
    {
        public void ExportAll(TestRunReport report, AggregatedStats stats, string outputDirectory)
        {
            if (report is null) throw new ArgumentNullException(nameof(report));
            if (stats is null) throw new ArgumentNullException(nameof(stats));
            if (string.IsNullOrWhiteSpace(outputDirectory)) throw new ArgumentException("Output directory is required.", nameof(outputDirectory));

            Directory.CreateDirectory(outputDirectory);

            ExportMatchesCsv(report, Path.Combine(outputDirectory, "matches.csv"));
            ExportSummaryCsv(stats, Path.Combine(outputDirectory, "summary.csv"));
            ExportHtml(report, stats, Path.Combine(outputDirectory, "report.html"));
        }

        public void ExportMatchesCsv(TestRunReport report, string filePath)
        {
            var sb = new StringBuilder();
            sb.AppendLine("MatchId,Seed,FinalStatus,Winner,EndReason,TotalFrames,Score,DurationMs,ExitCode,WallClockMs");

            foreach (var r in report.AllResults.OrderBy(x => x.MatchId, StringComparer.OrdinalIgnoreCase))
            {
                var exitCode = r.ProcessResult?.ExitCode ?? -1;
                var wallClockMs = r.ProcessResult?.WallClockDuration.TotalMilliseconds ?? 0;

                sb.AppendLine(string.Join(",",
                    Csv(r.MatchId),
                    r.Seed.ToString(CultureInfo.InvariantCulture),
                    Csv(r.FinalStatus.ToString()),
                    Csv(r.Winner ?? string.Empty),
                    Csv(r.EndReason ?? string.Empty),
                    r.TotalFrames.ToString(CultureInfo.InvariantCulture),
                    r.Score.ToString(CultureInfo.InvariantCulture),
                    r.DurationMs.ToString(CultureInfo.InvariantCulture),
                    exitCode.ToString(CultureInfo.InvariantCulture),
                    wallClockMs.ToString("F0", CultureInfo.InvariantCulture)
                ));
            }

            File.WriteAllText(filePath, sb.ToString(), Encoding.UTF8);
        }

        public void ExportSummaryCsv(AggregatedStats stats, string filePath)
        {
            var sb = new StringBuilder();

            sb.AppendLine("Metric,Value");
            sb.AppendLine($"TotalMatches,{stats.TotalMatches}");
            sb.AppendLine($"CompletedMatches,{stats.CompletedMatches}");
            sb.AppendLine($"FailedMatches,{stats.FailedMatches}");
            sb.AppendLine($"SuccessRatePct,{stats.SuccessRate.ToString("F2", CultureInfo.InvariantCulture)}");
            sb.AppendLine($"AverageFrames,{stats.AverageFrames.ToString("F2", CultureInfo.InvariantCulture)}");
            sb.AppendLine($"AverageDurationMs,{stats.AverageDurationMs.ToString("F2", CultureInfo.InvariantCulture)}");
            sb.AppendLine($"AverageScore,{stats.AverageScore.ToString("F2", CultureInfo.InvariantCulture)}");
            sb.AppendLine($"AnomaliesCount,{stats.AnomaliesCount}");

            sb.AppendLine();
            sb.AppendLine("Status,Count");
            foreach (var kv in stats.StatusBreakdown.OrderByDescending(k => k.Value))
            {
                sb.AppendLine($"{Csv(kv.Key.ToString())},{kv.Value}");
            }

            sb.AppendLine();
            sb.AppendLine("Player,WinRatePct");
            foreach (var kv in stats.WinRateByPlayer.OrderByDescending(k => k.Value))
            {
                sb.AppendLine($"{Csv(kv.Key)},{kv.Value.ToString("F2", CultureInfo.InvariantCulture)}");
            }

            File.WriteAllText(filePath, sb.ToString(), Encoding.UTF8);
        }

        public void ExportHtml(TestRunReport report, AggregatedStats stats, string filePath)
        {
            var html = new StringBuilder();

            html.AppendLine("<!doctype html>");
            html.AppendLine("<html lang=\"en\">");
            html.AppendLine("<head>");
            html.AppendLine("  <meta charset=\"utf-8\" />");
            html.AppendLine("  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />");
            html.AppendLine("  <title>BattleCity Report</title>");
            html.AppendLine("  <style>");
            html.AppendLine("    body{font-family:Segoe UI,Arial,sans-serif;margin:20px;color:#222;}");
            html.AppendLine("    h1,h2{margin:0 0 10px 0;}");
            html.AppendLine("    .grid{display:grid;grid-template-columns:repeat(4,minmax(120px,1fr));gap:12px;margin:14px 0 22px;}");
            html.AppendLine("    .card{border:1px solid #ddd;border-radius:8px;padding:12px;background:#fafafa;}");
            html.AppendLine("    .k{font-size:12px;color:#666;} .v{font-size:22px;font-weight:700;}");
            html.AppendLine("    table{width:100%;border-collapse:collapse;margin-top:8px;}");
            html.AppendLine("    th,td{border:1px solid #ddd;padding:6px 8px;font-size:13px;text-align:left;}");
            html.AppendLine("    th{background:#f2f2f2;}");
            html.AppendLine("    .ok{color:#0a7d2c;font-weight:600;} .bad{color:#b00020;font-weight:600;}");
            html.AppendLine("  </style>");
            html.AppendLine("</head>");
            html.AppendLine("<body>");

            html.AppendLine("  <h1>BattleCity Test Report</h1>");
            html.AppendLine($"  <p>Generated: {DateTime.Now:yyyy-MM-dd HH:mm:ss}</p>");

            html.AppendLine("  <div class=\"grid\">");
            Card(html, "Total", stats.TotalMatches.ToString(CultureInfo.InvariantCulture));
            Card(html, "Completed", stats.CompletedMatches.ToString(CultureInfo.InvariantCulture));
            Card(html, "Failed", stats.FailedMatches.ToString(CultureInfo.InvariantCulture));
            Card(html, "Success %", stats.SuccessRate.ToString("F2", CultureInfo.InvariantCulture));
            Card(html, "Avg Frames", stats.AverageFrames.ToString("F2", CultureInfo.InvariantCulture));
            Card(html, "Avg Duration (ms)", stats.AverageDurationMs.ToString("F2", CultureInfo.InvariantCulture));
            Card(html, "Anomalies", stats.AnomaliesCount.ToString(CultureInfo.InvariantCulture));
            Card(html, "Run Duration", report.TotalDuration.ToString());
            html.AppendLine("  </div>");

            html.AppendLine("  <h2>Status Breakdown</h2>");
            html.AppendLine("  <table><thead><tr><th>Status</th><th>Count</th></tr></thead><tbody>");
            foreach (var kv in stats.StatusBreakdown.OrderByDescending(x => x.Value))
            {
                html.AppendLine($"<tr><td>{H(kv.Key.ToString())}</td><td>{kv.Value}</td></tr>");
            }
            html.AppendLine("  </tbody></table>");

            html.AppendLine("  <h2>Win Rate By Player</h2>");
            html.AppendLine("  <table><thead><tr><th>Player</th><th>Win Rate %</th></tr></thead><tbody>");
            foreach (var kv in stats.WinRateByPlayer.OrderByDescending(x => x.Value))
            {
                html.AppendLine($"<tr><td>{H(kv.Key)}</td><td>{kv.Value.ToString("F2", CultureInfo.InvariantCulture)}</td></tr>");
            }
            html.AppendLine("  </tbody></table>");

            html.AppendLine("  <h2>Matches</h2>");
            html.AppendLine("  <table><thead><tr>" +
                            "<th>MatchId</th><th>Seed</th><th>Status</th><th>Winner</th><th>EndReason</th>" +
                            "<th>Frames</th><th>DurationMs</th><th>ExitCode</th></tr></thead><tbody>");

            foreach (var r in report.AllResults.OrderBy(x => x.MatchId, StringComparer.OrdinalIgnoreCase))
            {
                var css = r.IsFailure ? "bad" : "ok";
                html.AppendLine(
                    $"<tr>" +
                    $"<td>{H(r.MatchId)}</td>" +
                    $"<td>{r.Seed}</td>" +
                    $"<td class=\"{css}\">{H(r.FinalStatus.ToString())}</td>" +
                    $"<td>{H(r.Winner ?? string.Empty)}</td>" +
                    $"<td>{H(r.EndReason ?? string.Empty)}</td>" +
                    $"<td>{r.TotalFrames}</td>" +
                    $"<td>{r.DurationMs}</td>" +
                    $"<td>{(r.ProcessResult?.ExitCode ?? -1)}</td>" +
                    $"</tr>");
            }

            html.AppendLine("  </tbody></table>");
            html.AppendLine("</body></html>");

            File.WriteAllText(filePath, html.ToString(), Encoding.UTF8);
        }

        private static void Card(StringBuilder sb, string key, string value)
        {
            sb.AppendLine("    <div class=\"card\">");
            sb.AppendLine($"      <div class=\"k\">{H(key)}</div>");
            sb.AppendLine($"      <div class=\"v\">{H(value)}</div>");
            sb.AppendLine("    </div>");
        }

        private static string Csv(string value)
        {
            if (value is null) return "\"\"";
            var escaped = value.Replace("\"", "\"\"");
            return $"\"{escaped}\"";
        }

        private static string H(string value) => WebUtility.HtmlEncode(value ?? string.Empty);
    }
}
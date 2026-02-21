using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace BattleCity.Orchestrator.Core
{
    public class MatchResult
    {
        [JsonIgnore]
        public string MatchId {get;set;} = string.Empty;

        [JsonPropertyName("seed")]
        public int Seed{get;set;}

        [JsonPropertyName("totalFrames")]
        public int TotalFrames{get;set;}

        [JsonPropertyName("frames")]
        public int Frames
        {
            get => TotalFrames;
            set => TotalFrames = value;
        }

        [JsonPropertyName("score")]
        public int Score { get; set; }

        [JsonPropertyName("durationMs")]
        public long DurationMs{get;set;}

        [JsonPropertyName("winner")]
        public string? Winner{get;set;}

        [JsonPropertyName("endReason")]
        public string EndReason{get;set;} = string.Empty;

        [JsonIgnore]
        public GameEndReason EndReasonParsed => Enum.TryParse<GameEndReason>(EndReason, true, out var r)? r : GameEndReason.Unknown;

        [JsonPropertyName("players")]
        public List<PlayerResult> Players {get; set;} = new();

        [JsonIgnore]
        public MatchProcessResult? ProcessResult { get; set; }


        [JsonIgnore]
        public MatchFinalStatus FinalStatus
        {
            get
            {
                if(ProcessResult is null)
                {
                    return MatchFinalStatus.Unknown;
                }
                return ProcessResult.Outcome switch
                {
                    MatchOutcome.Crashed => MatchFinalStatus.Crashed,
                    MatchOutcome.TimedOut => MatchFinalStatus.TimedOut,
                    MatchOutcome.LaunchFailed => MatchFinalStatus.LaunchFailed,
                    MatchOutcome.CompletedNoOutput => MatchFinalStatus.NoOutput,
                    MatchOutcome.Completed => CategorizeCompleted(),
                    _ => MatchFinalStatus.Unknown
                };
            }
        }
        private MatchFinalStatus CategorizeCompleted()
        {
            if (string.IsNullOrEmpty(Winner) || string.Equals(Winner, "Draw", StringComparison.OrdinalIgnoreCase))
            {
                return MatchFinalStatus.Draw;
            }
            if (EndReasonParsed == GameEndReason.MaxFramesReached)
            {
                return MatchFinalStatus.CompletedByTimeout;
            }
            return MatchFinalStatus.CompletedNormally;
        }

        public PlayerResult? GetWinner()
        {
            if (string.IsNullOrEmpty(Winner) || string.Equals(Winner, "Draw", StringComparison.OrdinalIgnoreCase))
            {
                return null;
            }
            return Players.Find(p=> string.Equals(p.Id, Winner, StringComparison.OrdinalIgnoreCase));
        }

        public List<PlayerResult> GetLosers()
        {
            return Players.FindAll(p=> !string.Equals(p.Id, Winner, StringComparison.OrdinalIgnoreCase));
        }

        [JsonIgnore]
        public bool IsFailure => FinalStatus is MatchFinalStatus.Crashed or MatchFinalStatus.TimedOut or MatchFinalStatus.LaunchFailed or MatchFinalStatus.NoOutput or MatchFinalStatus.Unknown;

        public bool IsAnomalous(int expectedMaxFrames)
        {
            if (TotalFrames >= expectedMaxFrames)
            {
                return true;
            }
            if (Players.Count > 0 && Players.TrueForAll(p=> p.LivesRemaining == 0))
            {
                return true;
            }
            return false;
        }
    }
    
    public class PlayerResult
    {
        [JsonPropertyName("id")]
        public string Id{get; set;} = string.Empty;

        [JsonPropertyName("livesRemaining")]
        public int LivesRemaining {get;set;}

        [JsonPropertyName("alive")]
        public bool Alive{get;set;}

        public override string ToString() => $"{Id}: Lives={LivesRemaining}, Alive={Alive}";

    }

    public enum GameEndReason
    {
        Unknown,
        BaseDestroyed,
        AllTanksDestroyed,
        MaxFramesReached,
        Error
    }

    public enum MatchFinalStatus
    {
        Unknown,
        CompletedNormally,
        CompletedByTimeout,
        Draw,
        NoOutput,
        Crashed,
        TimedOut,
        LaunchFailed
    }

    public static class MatchResultFactory
    {
        private static readonly JsonSerializerOptions JsonOptions = new()
        {
            PropertyNameCaseInsensitive = true,
            ReadCommentHandling = JsonCommentHandling.Skip,
            AllowTrailingCommas = true
        };

        public static MatchResult FromProcessResult(MatchProcessResult processResult)
        {
             MatchResult result;

            if (processResult.HasOutputJson)
            {
                try
                {
                    result = FromJsonFile(processResult.OutputJsonPath);
                }
                catch (Exception ex)
                {
                    result = new MatchResult();
                    result.EndReason = $"JsonParseError: {ex.Message}";
                }
            }
            else
            {
                result = new MatchResult();
            }

            result.MatchId = processResult.MatchId;
            result.ProcessResult = processResult;

            if (result.DurationMs <= 0)
            {
                result.DurationMs = (long)Math.Max(0, processResult.WallClockDuration.TotalMilliseconds);
            }

            if (string.IsNullOrWhiteSpace(result.EndReason))
            {
                result.EndReason = processResult.Outcome.ToString();
            }

            return result;
        }

        public static MatchResult FromJsonFile(string filePath)
        {
            if (!File.Exists(filePath))
            {
                throw new FileNotFoundException($"Result JSON not found: {filePath}", filePath);
            }
            var json = File.ReadAllText(filePath);
            return FromJsonString(json);
        }

        public static MatchResult FromJsonString(string json)
        {
            return JsonSerializer.Deserialize<MatchResult>(json, JsonOptions) ?? throw new InvalidOperationException("Deserialization returned null");
        }

        public static MatchResult Failed(MatchProcessResult processResult, string reason)
        {
            return new MatchResult
            {
                MatchId = processResult.MatchId,
                ProcessResult = processResult,
                EndReason = reason
            };
        }
    }
}
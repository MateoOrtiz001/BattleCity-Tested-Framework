using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace BattleCity.Orchestrator.Core
{
    /// <summary>
    /// Configuración para lanzar una partida individual de BattleCityHeadless.
    /// </summary>
    public class MatchConfig
    {
        /// <summary>Seed para reproducibilidad. Null = aleatorio.</summary>
        public int? Seed { get; set; }

        /// <summary>Máximo de frames antes de cortar la partida.</summary>
        public int MaxFrames { get; set; } = 10_000;

        /// <summary>Ruta donde el proceso escribirá el JSON de resultados.</summary>
        public string OutputJsonPath { get; set; } = string.Empty;

        /// <summary>Ruta opcional al archivo .txt de cheats.</summary>
        public string? CheatsFilePath { get; set; }

        /// <summary>Timeout máximo real (wall-clock) para el proceso.</summary>
        public TimeSpan ProcessTimeout { get; set; } = TimeSpan.FromSeconds(60);

        /// <summary>Argumentos extra que se pasan tal cual al proceso.</summary>
        public Dictionary<string, string> ExtraArguments { get; set; } = new();

        /// <summary>Identificador legible de esta partida (para logs).</summary>
        public string MatchId { get; set; } = Guid.NewGuid().ToString("N")[..8];

        /// <summary>Crea una copia superficial de este config.</summary>
        public MatchConfig Clone()
        {
            return new MatchConfig
            {
                Seed = Seed,
                MaxFrames = MaxFrames,
                OutputJsonPath = OutputJsonPath,
                CheatsFilePath = CheatsFilePath,
                ProcessTimeout = ProcessTimeout,
                ExtraArguments = new Dictionary<string, string>(ExtraArguments),
                MatchId = MatchId
            };
        }
    }

    /// <summary>
    /// Resultado crudo de la ejecución del proceso (antes de parsear el JSON).
    /// </summary>
    public class MatchProcessResult
    {
        public string MatchId { get; init; } = string.Empty;
        public MatchOutcome Outcome { get; init; }
        public int ExitCode { get; init; }
        public TimeSpan WallClockDuration { get; init; }
        public string StandardOutput { get; init; } = string.Empty;
        public string StandardError { get; init; } = string.Empty;
        public string OutputJsonPath { get; init; } = string.Empty;

        /// <summary>True si el JSON de resultados existe en disco.</summary>
        public bool HasOutputJson => !string.IsNullOrEmpty(OutputJsonPath) && File.Exists(OutputJsonPath);
    }

    public enum MatchOutcome
    {
        Completed,
        CompletedNoOutput,
        Crashed,
        TimedOut,
        LaunchFailed
    }

    /// <summary>
    /// Lanza una instancia de BattleCityHeadless como proceso hijo,
    /// captura su salida y determina si completó, crasheó o se colgó.
    /// </summary>
    public class MatchProcess
    {
        private readonly string _executablePath;

        public MatchProcess(string executablePath)
        {
            if (!File.Exists(executablePath))
                throw new FileNotFoundException(
                    $"Headless executable not found: {executablePath}", executablePath);

            _executablePath = Path.GetFullPath(executablePath);
        }

        /// <summary>
        /// Ejecuta una partida con la configuración dada.
        /// </summary>
        public async Task<MatchProcessResult> RunAsync(
            MatchConfig config,
            CancellationToken cancellationToken = default)
        {
            var stopwatch = Stopwatch.StartNew();
            var arguments = BuildArguments(config);

            var stdoutBuilder = new StringBuilder();
            var stderrBuilder = new StringBuilder();

            var startInfo = new ProcessStartInfo
            {
                FileName = _executablePath,
                Arguments = arguments,
                UseShellExecute = false,
                CreateNoWindow = true,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                StandardOutputEncoding = Encoding.UTF8,
                StandardErrorEncoding = Encoding.UTF8,
                WorkingDirectory = Path.GetDirectoryName(_executablePath) ?? ""
            };

            Process? process = null;

            try
            {
                process = new Process { StartInfo = startInfo };

                // Usar TaskCompletionSource para stdout/stderr completos
                var stdoutDone = new TaskCompletionSource<bool>();
                var stderrDone = new TaskCompletionSource<bool>();

                process.OutputDataReceived += (_, e) =>
                {
                    if (e.Data is not null)
                        stdoutBuilder.AppendLine(e.Data);
                    else
                        stdoutDone.TrySetResult(true);
                };

                process.ErrorDataReceived += (_, e) =>
                {
                    if (e.Data is not null)
                        stderrBuilder.AppendLine(e.Data);
                    else
                        stderrDone.TrySetResult(true);
                };

                if (!process.Start())
                {
                    return MakeResult(config, MatchOutcome.LaunchFailed, -1,
                        stopwatch.Elapsed, "", "Process.Start() returned false");
                }

                process.BeginOutputReadLine();
                process.BeginErrorReadLine();

                // Esperar a que termine o se exceda el timeout
                bool exited = await WaitForExitAsync(process, config.ProcessTimeout, cancellationToken);

                if (!exited)
                {
                    KillProcess(process);
                    stopwatch.Stop();

                    // Dar tiempo a que se vacíen los buffers
                    await Task.WhenAny(
                        Task.WhenAll(stdoutDone.Task, stderrDone.Task),
                        Task.Delay(1000));

                    return MakeResult(config, MatchOutcome.TimedOut, -1,
                        stopwatch.Elapsed,
                        stdoutBuilder.ToString(),
                        stderrBuilder.ToString());
                }

                // Esperar a que se vacíen los buffers de stdout/stderr
                await Task.WhenAny(
                    Task.WhenAll(stdoutDone.Task, stderrDone.Task),
                    Task.Delay(3000));

                stopwatch.Stop();

                int exitCode = process.ExitCode;
                var outcome = DetermineOutcome(exitCode, config.OutputJsonPath);

                return MakeResult(config, outcome, exitCode,
                    stopwatch.Elapsed,
                    stdoutBuilder.ToString(),
                    stderrBuilder.ToString());
            }
            catch (OperationCanceledException)
            {
                KillProcess(process);
                stopwatch.Stop();

                return MakeResult(config, MatchOutcome.TimedOut, -1,
                    stopwatch.Elapsed,
                    stdoutBuilder.ToString(),
                    stderrBuilder.ToString());
            }
            catch (Exception ex)
            {
                KillProcess(process);
                stopwatch.Stop();

                return MakeResult(config, MatchOutcome.LaunchFailed, -1,
                    stopwatch.Elapsed, "",
                    $"Exception launching process: {ex.Message}");
            }
            finally
            {
                process?.Dispose();
            }
        }

        private static string BuildArguments(MatchConfig config)
        {
            var args = new List<string>();

            if (config.Seed.HasValue)
                args.Add($"--seed {config.Seed.Value}");

            args.Add($"--maxFrames {config.MaxFrames}");

            if (!string.IsNullOrWhiteSpace(config.OutputJsonPath))
                args.Add($"--output \"{config.OutputJsonPath}\"");

            if (!string.IsNullOrWhiteSpace(config.CheatsFilePath))
                args.Add($"--cheats \"{config.CheatsFilePath}\"");

            foreach (var kvp in config.ExtraArguments)
            {
                args.Add($"--{kvp.Key} \"{kvp.Value}\"");
            }

            return string.Join(" ", args);
        }

        private static async Task<bool> WaitForExitAsync(
            Process process,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            using var timeoutCts = new CancellationTokenSource(timeout);
            using var linkedCts = CancellationTokenSource.CreateLinkedTokenSource(
                timeoutCts.Token, cancellationToken);

            try
            {
                await process.WaitForExitAsync(linkedCts.Token);
                return true;
            }
            catch (OperationCanceledException)
            {
                if (cancellationToken.IsCancellationRequested)
                    throw;
                return false;
            }
        }

        private static void KillProcess(Process? process)
        {
            if (process is null)
                return;

            try
            {
                if (!process.HasExited)
                    process.Kill(entireProcessTree: true);
            }
            catch (InvalidOperationException)
            {
                // El proceso ya terminó
            }
            catch (System.ComponentModel.Win32Exception)
            {
                // Sin permisos o proceso ya no existe
            }
        }

        private static MatchOutcome DetermineOutcome(int exitCode, string outputJsonPath)
        {
            if (exitCode != 0)
                return MatchOutcome.Crashed;

            bool jsonExists = !string.IsNullOrEmpty(outputJsonPath) && File.Exists(outputJsonPath);
            return jsonExists ? MatchOutcome.Completed : MatchOutcome.CompletedNoOutput;
        }

        private static MatchProcessResult MakeResult(
            MatchConfig config,
            MatchOutcome outcome,
            int exitCode,
            TimeSpan duration,
            string stdout,
            string stderr)
        {
            return new MatchProcessResult
            {
                MatchId = config.MatchId,
                Outcome = outcome,
                ExitCode = exitCode,
                WallClockDuration = duration,
                StandardOutput = stdout,
                StandardError = stderr,
                OutputJsonPath = config.OutputJsonPath
            };
        }
    }
}
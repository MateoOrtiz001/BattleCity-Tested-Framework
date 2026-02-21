using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace BattleCity.Orchestrator.Cheats
{
    public enum CheatFileFormat
    {
        KeyValue, // key=value
        Command   // set key value / enable flag
    }

    public sealed class CheatScriptBuilder
    {
        private readonly List<(int Frame, string Command)> _scheduled = new();
        private readonly Dictionary<string, string> _kv = new(StringComparer.OrdinalIgnoreCase);
        private readonly HashSet<string> _flags = new(StringComparer.OrdinalIgnoreCase);
        private readonly List<string> _comments = new();
        private readonly List<string> _rawLines = new();

        public CheatScriptBuilder Set(string key, object? value)
        {
            if (string.IsNullOrWhiteSpace(key))
                throw new ArgumentException("Key is required.", nameof(key));

            _kv[key.Trim()] = value?.ToString() ?? string.Empty;
            return this;
        }

        public CheatScriptBuilder Flag(string name)
        {
            if (string.IsNullOrWhiteSpace(name))
                throw new ArgumentException("Flag name is required.", nameof(name));

            _flags.Add(name.Trim());
            return this;
        }

        public CheatScriptBuilder Comment(string text)
        {
            if (!string.IsNullOrWhiteSpace(text))
                _comments.Add(text.Trim());
            return this;
        }

        public CheatScriptBuilder Raw(string line)
        {
            if (!string.IsNullOrWhiteSpace(line))
                _rawLines.Add(line);
            return this;
        }

        public CheatScriptBuilder CommandAt(int frame, string command)
        {
            if (frame < 0)
                throw new ArgumentException("Frame must be >= 0.", nameof(frame));
            if (string.IsNullOrWhiteSpace(command))
                throw new ArgumentException("Command is required.", nameof(command));

            _scheduled.Add((frame, command.Trim()));
            return this;
        }

        public string Build(CheatFileFormat format = CheatFileFormat.KeyValue)
        {
            var sb = new StringBuilder();

            sb.AppendLine("# BattleCity cheat script");
            sb.AppendLine($"# Generated: {DateTime.Now:yyyy-MM-dd HH:mm:ss}");
            foreach (var c in _comments)
                sb.AppendLine($"# {c}");
            sb.AppendLine();

            switch (format)
            {
                case CheatFileFormat.KeyValue:
                    foreach (var f in _flags.OrderBy(x => x, StringComparer.OrdinalIgnoreCase))
                        sb.AppendLine($"{f}=true");

                    foreach (var kv in _kv.OrderBy(x => x.Key, StringComparer.OrdinalIgnoreCase))
                        sb.AppendLine($"{kv.Key}={kv.Value}");

                    foreach (var raw in _rawLines)
                        sb.AppendLine(raw);
                    break;

                case CheatFileFormat.Command:
                    foreach (var f in _flags.OrderBy(x => x, StringComparer.OrdinalIgnoreCase))
                        sb.AppendLine($"enable {f}");

                    foreach (var kv in _kv.OrderBy(x => x.Key, StringComparer.OrdinalIgnoreCase))
                        sb.AppendLine($"set {kv.Key} {Escape(kv.Value)}");

                    foreach (var raw in _rawLines)
                        sb.AppendLine(raw);
                    break;

                default:
                    throw new ArgumentOutOfRangeException(nameof(format), format, "Unknown format.");
            }

            return sb.ToString();
        }

        public void Save(string filePath, CheatFileFormat format = CheatFileFormat.KeyValue)
        {
            if (string.IsNullOrWhiteSpace(filePath))
                throw new ArgumentException("Output file path is required.", nameof(filePath));

            var fullPath = Path.GetFullPath(filePath);
            var dir = Path.GetDirectoryName(fullPath);
            if (!string.IsNullOrWhiteSpace(dir))
                Directory.CreateDirectory(dir);

            File.WriteAllText(fullPath, Build(format), Encoding.UTF8);
        }

        public string BuildScheduledScript()
        {
            var sb = new StringBuilder();
            sb.AppendLine("# BattleCity scheduled cheat script");
            sb.AppendLine($"# Generated: {DateTime.Now:yyyy-MM-dd HH:mm:ss}");
            foreach (var c in _comments)
                sb.AppendLine($"# {c}");
            sb.AppendLine();

            foreach (var item in _scheduled.OrderBy(x => x.Frame))
                sb.AppendLine($"{item.Frame} {item.Command}");

            return sb.ToString();
        }

        public void SaveScheduledScript(string filePath)
        {
            if (string.IsNullOrWhiteSpace(filePath))
                throw new ArgumentException("Output file path is required.", nameof(filePath));

            var fullPath = Path.GetFullPath(filePath);
            var dir = Path.GetDirectoryName(fullPath);
            if (!string.IsNullOrWhiteSpace(dir))
                Directory.CreateDirectory(dir);

            File.WriteAllText(fullPath, BuildScheduledScript(), Encoding.UTF8);
        }

        public static bool TryParseSet(string input, out string key, out string value)
        {
            key = string.Empty;
            value = string.Empty;
            if (string.IsNullOrWhiteSpace(input)) return false;

            var idx = input.IndexOf('=');
            if (idx <= 0 || idx >= input.Length - 1) return false;

            key = input[..idx].Trim();
            value = input[(idx + 1)..].Trim();
            return key.Length > 0;
        }

        private static string Escape(string value)
        {
            if (string.IsNullOrEmpty(value)) return "\"\"";
            if (value.Contains(' ') || value.Contains('"'))
                return $"\"{value.Replace("\"", "\\\"")}\"";
            return value;
        }
    }
}
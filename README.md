# BattleCity-Tested-Framework

A Battle City engine with a comprehensive test orchestration tool designed for automated verification, stress testing and replay analysis.

---

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Project Structure](#project-structure)
- [Prerequisites](#prerequisites)
- [Build Instructions](#build-instructions)
- [Modules](#modules)
  - [BattleCity.Engine](#battlecityengine)
  - [BattleCity.Headless](#battlecityheadless)
  - [BattleCity.GUI](#battlecitygui)
  - [BattleCity.Orchestrator](#battlecityorchestrator)
- [Game Mechanics](#game-mechanics)
- [Levels](#levels)
- [AI Agents (Policies)](#ai-agents-policies)
- [Cheat System](#cheat-system)
- [Usage Examples](#usage-examples)
- [Output Format](#output-format)
- [License](#license)

---

## Overview

BattleCity-Tested-Framework is a deterministic simulation of the classic Battle City tank game, built as a multi-component C++17 / C# project. The framework is designed for:

- **Automated testing**: Run hundreds of matches in parallel with different seeds and configurations.
- **Reproducibility**: Given the same seed, level, and policies, a match always produces the same result.
- **Stress testing**: Spawn large numbers of tanks via cheat scripts to test engine robustness.
- **Visual replay**: Replay any match result in a GUI window (raylib) to visually verify behavior.
- **Statistical analysis**: The Orchestrator generates CSV, HTML, and text reports with win rates, anomalies, and status breakdowns.

---

## Architecture

The system is composed of four interconnected modules:

```
┌─────────────────────────────────────────────────────────┐
│                  BattleCity.Orchestrator (.NET 7)        │
│  Launches N parallel instances of BattleCityHeadless    │
│  Collects JSON results → CSV / HTML / TXT reports       │
│  Commands: run, rerun-failed, build-cheats              │
└────────────────────────┬────────────────────────────────┘
                         │ spawns processes
          ┌──────────────▼──────────────┐
          │    BattleCityHeadless (C++) │
          │  CLI runner, no GUI         │
          │  --seed, --level, --cheats  │
          │  Outputs result JSON        │
          └──────────────┬──────────────┘
                         │ uses
          ┌──────────────▼──────────────┐
          │     BattleCityEngine (C++)  │
          │  GameState, Entities,       │
          │  Agents, PathFinder,        │
          │  CheatManager               │
          └──────────────┬──────────────┘
                         │ also used by
          ┌──────────────▼──────────────┐
          │      BattleCityGUI (C++)    │
          │  Visual replay with raylib  │
          │  --result to replay a match │
          └─────────────────────────────┘
```

---

## Project Structure

```
BattleCity-Tested-Framework/
├── CMakeLists.txt                  # Build configuration (C++ targets)
├── BattleCity.sln                  # Visual Studio solution
├── README.md
├── LICENSE
├── src/
│   ├── BattleCity.Engine/          # Core simulation library (static lib)
│   │   ├── include/
│   │   │   ├── core/
│   │   │   │   ├── GameState.h     # Main game state machine
│   │   │   │   ├── Action.h        # Action types (Stop, Move, Fire)
│   │   │   │   └── CheatManager.h  # Runtime cheat command processor
│   │   │   ├── entities/
│   │   │   │   ├── Tank.h          # Tank entity (health, lives, position)
│   │   │   │   ├── Bullet.h        # Bullet projectile
│   │   │   │   ├── Wall.h          # Wall (brick/steel)
│   │   │   │   └── Base.h          # Team base (destroy to win)
│   │   │   ├── agents/
│   │   │   │   ├── IAgent.h        # Abstract agent interface
│   │   │   │   ├── ScriptedEnemyAgent.h  # 5 AI behaviors
│   │   │   │   └── PathFinder.h    # A* pathfinding
│   │   │   ├── map/
│   │   │   │   ├── Level1.cpp      # Level layouts (1-5)
│   │   │   │   ├── Level2.cpp
│   │   │   │   ├── Level3.cpp
│   │   │   │   ├── Level4.cpp
│   │   │   │   └── Level5.cpp
│   │   │   └── utils/
│   │   │       └── Utils.h         # ManhattanDistance, Clamp, etc.
│   │   ├── src/                    # Implementation files (.cpp)
│   │   ├── simulator/
│   │   │   ├── Runner.h            # Match orchestration (StartMatch, StepMatch, RunMatch)
│   │   │   └── Runner.cpp
│   │   └── external/
│   │       └── json.hpp            # nlohmann/json (single header)
│   │
│   ├── BattleCity.Headless/        # CLI executable (no GUI)
│   │   └── main.cpp                # Entry point with CLI argument parsing
│   │
│   ├── BattleCity.GUI/             # Visual replay executable (raylib)
│   │   └── src/
│   │       ├── main.cpp            # Entry point, window loop, replay from JSON
│   │       ├── Render2D.h/cpp      # 2D grid renderer
│   │       └── InputMap.h/cpp      # Keyboard input mapping
│   │
│   └── BattleCity.Orchestrator/    # Test runner & analysis (.NET 7)
│       ├── Program.cs              # CLI: run, rerun-failed, build-cheats
│       ├── BattleCity.Orchestrator.csproj
│       ├── Core/
│       │   ├── TestRunner.cs       # Parallel match executor
│       │   ├── MatchProcess.cs     # Process launcher for Headless
│       │   └── MatchResult.cs      # JSON result deserialization
│       ├── Analysis/
│       │   ├── StatsAggregator.cs  # Win rates, anomalies, averages
│       │   └── ReportExporter.cs   # CSV, HTML, TXT report generation
│       └── Cheats/
│           └── CheatScriptBuilder.cs  # Programmatic cheat file generator
│
├── build/                          # CMake build output
│   └── Debug/ / Release/          # Compiled executables
│
├── integration-smoke/              # Sample test run results
│   ├── matches.csv
│   ├── report.html
│   ├── summary.csv
│   ├── summary.txt
│   └── result_seed_*.json
│
└── orchestrator-smoke/             # Sample orchestrator run results
```

---

## Prerequisites

| Component | Requirement |
|-----------|-------------|
| **C++ Compiler** | MSVC 2019+ or GCC/Clang with C++17 support |
| **CMake** | 3.5 or higher |
| **.NET SDK** | 7.0+ (only for the Orchestrator) |
| **raylib** | Auto-downloaded by CMake via FetchContent |

---

## Build Instructions

### C++ Components (Engine, Headless, GUI)

```bash
# From the project root
mkdir build && cd build
cmake ..
cmake --build . --config Debug
```

This produces:
- `build/Debug/BattleCityHeadless.exe` — Headless CLI runner
- `build/Debug/BattleCityGUI.exe` — Visual GUI with raylib

For Release builds:
```bash
cmake --build . --config Release
```

### .NET Orchestrator

```bash
cd src/BattleCity.Orchestrator
dotnet build
dotnet run -- --help
```

---

## Modules

### BattleCity.Engine

The core simulation library compiled as a **static library** (`BattleCityEngine`). Contains all game logic independent of any presentation layer.

**Key Components:**

| Class | Description |
|-------|-------------|
| `GameState` | Central game state machine. Manages the board, tanks, bullets, walls, and bases. Processes each frame: move bullets, check collisions, detect game-over conditions. |
| `Tank` | Entity with position, direction, health (3 HP), lives (5), team ('A'/'B'). Supports respawn at original spawn point. |
| `Bullet` | Projectile that moves one cell per frame in a fixed direction. Deactivated on collision. |
| `Wall` | Obstacle; `brick` walls are destructible (3 HP), `steel` walls are indestructible. |
| `Base` | Each team's base (1 HP). Destroying the enemy base wins the game. |
| `Action` | Value type representing a tank's decision: `Stop`, `Move(direction)`, or `Fire()`. |
| `CheatManager` | Processes cheat commands at runtime (spawn tanks, kill, heal, modify walls, etc.). |
| `Runner` | High-level match controller. Configures a match, loops through frames, delegates decisions to agents, executes scheduled cheats, and exports results to JSON. |

**Game Loop (per frame):**
1. Execute scheduled cheats for the current frame
2. Each alive tank's agent produces an `Action` (via `IAgent::Decide`)
3. Actions are applied (move or fire)
4. `GameState::Update()` advances the simulation:
   - Move bullets
   - Check bullet-wall, bullet-tank, bullet-base collisions
   - Remove destroyed walls
   - Check game-over conditions (base destroyed, all tanks dead, tick limit reached)

### BattleCity.Headless

A **command-line executable** that runs a complete match without any graphical output. Ideal for automated testing and batch execution by the Orchestrator.

**CLI Arguments:**

| Argument | Default | Description |
|----------|---------|-------------|
| `--seed <N>` | Random | Seed for deterministic RNG |
| `--level <name>` | `level1` | Level name: `level1` through `level5` |
| `--maxFrames <N>` | `500` | Maximum frames before forced draw |
| `--tickRate <N>` | `10` | Tick rate (used in results metadata) |
| `--output <path>` | `results/result<seed>.json` | Path for the JSON result file |
| `--cheats <path>` | _(none)_ | Path to a scheduled cheat script file |
| `--teamAPolicy <type>` | `attack_base` | Default AI policy for Team A |
| `--teamBPolicy <type>` | `attack_base` | Default AI policy for Team B |
| `--tankPolicy <id:type>` | _(none)_ | Per-tank policy override (repeatable) |

### BattleCity.GUI

A **graphical application** built with [raylib](https://www.raylib.com/) that renders the simulation in real time on a 2D grid.

**Features:**
- Real-time rendering of the game board (tanks, walls, bullets, bases)
- Accepts the same CLI arguments as Headless
- **Replay mode**: Pass `--result <path.json>` to replay a previous match with the exact same configuration (seed, policies, cheats)
- HUD showing current frame and score
- Game-over overlay with winner

**CLI Arguments (same as Headless, plus):**

| Argument | Description |
|----------|-------------|
| `--result <path>` | Load configuration from a result JSON to replay the match visually |

### BattleCity.Orchestrator

A **.NET 7 console application** that manages batch execution of headless matches, collects results, and generates reports.

**Commands:**

#### `run` — Execute batch matches
```bash
BattleCity.Orchestrator run --exe <path_to_headless.exe> [options]
```

| Option | Default | Description |
|--------|---------|-------------|
| `--exe <path>` | _(required)_ | Path to `BattleCityHeadless.exe` |
| `--count <N>` | `10` | Number of matches to run |
| `--start-seed <N>` | `0` | Starting seed value |
| `--max-frames <N>` | `10000` | Max frames per match |
| `--tick-rate <N>` | `10` | Tick rate |
| `--level <name>` | `level1` | Level name |
| `--parallel <N>` | CPU cores | Max parallel matches |
| `--timeout <seconds>` | `60` | Timeout per match |
| `--cheats <path>` | _(none)_ | Cheat script file |
| `--team-a-policy <type>` | `attack_base` | Team A agent policy |
| `--team-b-policy <type>` | `attack_base` | Team B agent policy |
| `--tank-policy <id:type>` | _(none)_ | Per-tank policy (repeatable) |
| `--output-dir <path>` | `results` | Output directory |
| `--retry` | `false` | Retry crashed matches once |
| `--verbose` | `false` | Detailed output |

#### `rerun-failed` — Re-execute failed matches
```bash
BattleCity.Orchestrator rerun-failed --exe <path> --output-dir <dir>
```
Reads error logs from a previous run and re-executes only the failed seeds.

#### `build-cheats` — Generate cheat script files
```bash
BattleCity.Orchestrator build-cheats --out <path> [options]
```

| Option | Description |
|--------|-------------|
| `--out <path>` | Output file (required) |
| `--preset <name>` | Preset: `empty-map`, `1v1`, `many-vs-many`, `destroy-base`, `destroy-base-a`, `destroy-base-b`, `stress` |
| `--at <frame:command>` | Custom command at a specific frame (repeatable) |
| `--many-count <N>` | Spawn N tanks per team at frame 1 |
| `--spawn-agent <type>` | Agent type for spawned tanks |
| `--print` | Print generated script to console |

**Generated Reports:**

| File | Format | Content |
|------|--------|---------|
| `matches.csv` | CSV | Per-match details (seed, status, winner, frames, score, duration) |
| `summary.csv` | CSV | Aggregated metrics (avg frames, win rates, anomalies) |
| `summary.txt` | Text | Human-readable summary |
| `report.html` | HTML | Visual dashboard with cards and tables |

---

## Game Mechanics

### Board
- Grid-based board where `(0,0)` is the **bottom-left** corner.
- Size is determined by the level layout (typically 13x13).

### Entities

| Entity | Symbol | Description |
|--------|--------|-------------|
| Tank A | `A` | Team A tank (blue in GUI). Health: 3, Lives: 5 |
| Tank B | `B` | Team B tank (red in GUI). Health: 3, Lives: 5 |
| Base A | `a` | Team A base (green). Health: 1 |
| Base B | `b` | Team B base (maroon). Health: 1 |
| Brick Wall | `X` | Destructible wall. Health: 3 |
| Steel Wall | `S` | Indestructible wall |
| Empty | ` ` | Traversable space |

### Directions
| Value | Direction |
|-------|-----------|
| 0 | North (+Y) |
| 1 | East (+X) |
| 2 | South (-Y) |
| 3 | West (-X) |

### Win Conditions
1. **Base Destroyed**: A team wins when the opponent's base is destroyed.
2. **All Tanks Dead**: A team wins when all enemy tanks lose all their lives.
3. **Draw by Timeout**: If `maxFrames` is reached, the match ends in a draw.

### Scoring
- Each tank kill awards **100 points**.

---

## Levels

The framework includes **5 built-in levels**, each a 13x13 grid:

| Level | Description |
|-------|-------------|
| `level1` | Symmetric map with brick corridors and two steel walls in the center |
| `level2` | Dense steel-walled center with open flanks |
| `level3` | Very open map with minimal walls — fast engagements |
| `level4` | Horizontal brick wall layers — requires shooting through |
| `level5` | Steel-dominant maze with a central corridor |

### Level 1 Layout Example
```
B    XbX    B    ← Team B tanks + base (b)
 X X XXX X X 
 X X     X X 
 X X X X X X 
 X X XSX X X    ← Steel wall (S) in center
     X X     
S XX     XX S    ← Steel corners
     X X     
 X X XSX X X 
 X X X X X X 
 X X     X X 
 X X XXX X X 
A    XaX    A    ← Team A tanks + base (a)
```

---

## AI Agents (Policies)

Each tank is controlled by an `IAgent` that produces an `Action` each frame. The `ScriptedEnemyAgent` implements 5 behavior policies:

| Policy | CLI Name | Description |
|--------|----------|-------------|
| **AttackBase** | `attack_base` | Moves toward the enemy base using Manhattan distance heuristic. 60% chance to fire when obstacle ahead, 90% if enemy in line of sight. Falls back to random legal move. |
| **Random** | `random` | Picks a random legal direction each frame. 20% chance to fire. |
| **Defensive** | `defensive` | Moves toward own base to defend it. Fires immediately when enemy is in line of sight. Only moves away from base if distance > 3. |
| **AStarAttack** | `astar_attack` | Uses A* pathfinding to find the cheapest target (enemy base or tank). Fires at walls blocking the path, shoots enemies on sight. |
| **Interceptor** | `interceptor` | Finds cheapest target via A* but currently stops (placeholder for future behavior). |

**Reproducibility**: Each agent uses a separate RNG seeded by `seed XOR (tankId * 2654435761)`, ensuring diverse but deterministic behavior across tanks.

---

## Cheat System

Cheats allow external manipulation of the game state during simulation. They can be:
1. **Inline**: Executed via `CheatManager::ExecuteCommand()` at any time.
2. **Scheduled**: Defined in a text file with `<frame> <command>` format.

### Cheat Script Format
```
# Lines starting with # are comments
<frame_number> <command> [args...]
```

### Available Commands

| Command | Arguments | Description |
|---------|-----------|-------------|
| `spawn_tank` | `<x> <y> <team> <agent_type>` | Spawn a single tank at position |
| `spawn_tanks` | `<count> <team> <agent_type>` | Spawn multiple tanks (auto-placed) |
| `kill_tank` | `<id>` | Kill a specific tank |
| `kill_all` | `<team>` | Kill all tanks of a team |
| `remove_tank` | `<id>` | Remove tank from simulation entirely |
| `remove_all_tanks` | `<team>` | Remove all tanks of a team |
| `heal_tank` | `<id> <amount>` | Heal a tank |
| `heal_all` | `<team> <amount>` | Heal all tanks of a team |
| `set_lives` | `<tankId> <lives>` | Set tank's remaining lives |
| `heal_base` | `<team> <amount>` | Heal a team's base |
| `set_base_health` | `<team> <health>` | Set base health directly |
| `destroy_base` | `<team>` | Immediately destroy a team's base |
| `add_wall` | `<x> <y> <type>` | Add a wall (`brick` or `steel`) |
| `remove_wall` | `<x> <y>` | Remove wall at position |
| `wall_type` | `<x> <y> <type>` | Change wall type |
| `all_walls_type` | `<type>` | Change all walls to a type |
| `clear_walls` | _(none)_ | Remove all walls |
| `spawn_bullet` | `<x> <y> <direction> <team>` | Spawn a bullet |
| `clear_bullets` | _(none)_ | Remove all bullets |
| `set_score` | `<score>` | Set the score |
| `set_tick_limit` | `<limit>` | Change the frame limit |
| `game_over` | `<winner: A\|B\|D>` | Force game over (`D` = draw) |
| `restart` | _(none)_ | Restart the match |
| `pause` | _(none)_ | Pause the simulation |
| `resume` | _(none)_ | Resume the simulation |

### Example Cheat Script (`cheat_stress.txt`)
```
# Stress test: spawn many tanks
1 spawn_tanks 50 A attack_base
1 spawn_tanks 50 B attack_base
```

---

## Usage Examples

### 1. Run a Single Headless Match

```bash
cd build
.\Debug\BattleCityHeadless.exe --seed 42 --level level1 --maxFrames 500 --output results/result_42.json
```

**Console Output:**
```
[Headless] Starting a match
[Headless] Seed: 42, Max Frames: 500, Tick Rate: 10
[Runner] Starting match with seed: 42
[Headless] Match finished. Saving results to results/result_42.json
[Runner] Match finished in 39 frames
[Runner] Score: 0
[Runner] Winner: Team B
[Runner] Cheats executed: 0
```

**Generated JSON (`results/result_42.json`):**
```json
{
    "cheats_executed": [],
    "cheats_failed": 0,
    "cheats_file": "",
    "cheats_total": 0,
    "frames": 39,
    "level": "level1",
    "max_frames": 500,
    "score": 0,
    "seed": 42,
    "tank_policies": {},
    "team_a_policy": "attack_base",
    "team_b_policy": "attack_base",
    "tick_rate": 10,
    "winner": "B"
}
```

### 2. Run with Different AI Policies

```bash
.\Debug\BattleCityHeadless.exe --seed 100 --level level3 --maxFrames 1000 \
    --teamAPolicy astar_attack --teamBPolicy defensive \
    --output results/result_100.json
```

**Result:** The match ends in a **Draw** after 1000 frames — the A* attackers couldn't break through the defensive agents in time.

### 3. Run with Cheat Script (Stress Test)

```bash
.\Debug\BattleCityHeadless.exe --seed 77 --level level1 --maxFrames 500 \
    --cheats cheat_stress.txt --output results/result_77_cheats.json
```

**Console Output:**
```
[Headless] Starting a match
[Headless] Seed: 77, Max Frames: 500, Tick Rate: 10
[Runner] Loaded cheat script: cheat_stress.txt
[Runner] Starting match with seed: 77
[Runner] Match finished in 81 frames
[Runner] Score: 115500
[Runner] Winner: Team B
[Runner] Cheats executed: 2
```

With 50+50 extra tanks spawned at frame 1, the match generates massive combat and 115,500 points in just 81 frames.

### 4. Visual Replay in GUI

```bash
# Replay a previous match result
.\Debug\BattleCityGUI.exe --result results/result_42.json

# Or run a new match visually
.\Debug\BattleCityGUI.exe --seed 42 --level level1
```

The GUI reads the seed, level, policies, and cheats from the JSON file and re-simulates the exact same match with visual rendering.

### 5. Batch Testing with Orchestrator

```bash
cd src/BattleCity.Orchestrator

# Run 100 matches in parallel
dotnet run -- run --exe ../../build/Debug/BattleCityHeadless.exe \
    --count 100 --start-seed 0 --level level1 \
    --output-dir ../../integration-smoke

# Rerun only failed matches
dotnet run -- rerun-failed --exe ../../build/Debug/BattleCityHeadless.exe \
    --output-dir ../../integration-smoke
```

**Sample Summary Output:**
```
========================================
         TEST RUN REPORT                
========================================
  Total matches:        2
  Completed:            2  (100.0%)
  Failed:               0
  Total duration:   00:00:00
  Avg match time:   0.229s
  Cancelled:        No
----------------------------------------
  CompletedNormally        2
----------------------------------------
  B            Wins:   2  Rate: 100.0%
========================================
```

### 6. Generate Cheat Scripts with Orchestrator

```bash
# Generate a stress test cheat file
dotnet run -- build-cheats --out cheats/stress.txt --preset stress --print

# Generate a custom cheat script
dotnet run -- build-cheats --out cheats/custom.txt \
    --at "10:spawn_tanks 3 A astar_attack" \
    --at "20:heal_all A 5" \
    --at "50:destroy_base B" \
    --print
```

### 7. Per-Tank Policy Override

```bash
# Tank 1 uses random, tank 2 uses A* attack, rest use default
.\Debug\BattleCityHeadless.exe --seed 42 --level level1 \
    --teamAPolicy attack_base --teamBPolicy defensive \
    --tankPolicy 1:random --tankPolicy 2:astar_attack
```

---

## Output Format

Every headless match produces a JSON result file with the following schema:

| Field | Type | Description |
|-------|------|-------------|
| `seed` | `int` | Seed used for RNG |
| `tick_rate` | `int` | Configured tick rate |
| `max_frames` | `int` | Frame limit |
| `level` | `string` | Level name used |
| `frames` | `int` | Actual frames elapsed |
| `score` | `int` | Total score (100 per kill) |
| `winner` | `string` | `"A"`, `"B"`, or `"Draw"` |
| `team_a_policy` | `string` | Default policy for Team A |
| `team_b_policy` | `string` | Default policy for Team B |
| `tank_policies` | `object` | Per-tank policy overrides `{"tankId": "policy"}` |
| `cheats_file` | `string` | Path to cheat script (if used) |
| `cheats_executed` | `array` | Log of executed cheats `[{frame, command, success}]` |
| `cheats_total` | `int` | Total cheat commands executed |
| `cheats_failed` | `int` | Number of failed cheat commands |

---

## License

See the [LICENSE](LICENSE) file for details.

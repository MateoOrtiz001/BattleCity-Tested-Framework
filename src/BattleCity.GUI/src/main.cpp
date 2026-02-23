#include <raylib.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <vector>
#include <utility>

#include "InputMap.h"
#include "Render2D.h"

#include "include/core/GameState.h"
#include "simulator/Runner.h"
#include "external/json.hpp"
#include "include/map/Level1.cpp"
#include "include/map/Level2.cpp"
#include "include/map/Level3.cpp"
#include "include/map/Level4.cpp"
#include "include/map/Level5.cpp"

using json = nlohmann::json;

bool LoadReplayConfig(
	const std::string& resultPath,
	unsigned int& seed,
	int& maxFrames,
	int& tickRate,
	std::string& levelName,
	std::string& cheatsPath,
	std::string& teamAPolicy,
	std::string& teamBPolicy,
	std::vector<std::pair<int, std::string>>& tankPolicySpecs
) {
	std::ifstream file(resultPath);
	if (!file.is_open()) {
		std::cerr << "[GUI] Error: Cannot open result file: " << resultPath << std::endl;
		return false;
	}

	try {
		json replay;
		file >> replay;

		if (replay.contains("seed") && replay["seed"].is_number_unsigned()) {
			seed = replay["seed"].get<unsigned int>();
		}
		if (replay.contains("max_frames") && replay["max_frames"].is_number_integer()) {
			maxFrames = replay["max_frames"].get<int>();
		}
		if (replay.contains("tick_rate") && replay["tick_rate"].is_number_integer()) {
			tickRate = replay["tick_rate"].get<int>();
		}
		if (replay.contains("level") && replay["level"].is_string()) {
			levelName = replay["level"].get<std::string>();
		}
		if (replay.contains("cheats_file") && replay["cheats_file"].is_string()) {
			cheatsPath = replay["cheats_file"].get<std::string>();
		}
		if (replay.contains("team_a_policy") && replay["team_a_policy"].is_string()) {
			teamAPolicy = replay["team_a_policy"].get<std::string>();
		}
		if (replay.contains("team_b_policy") && replay["team_b_policy"].is_string()) {
			teamBPolicy = replay["team_b_policy"].get<std::string>();
		}
		if (replay.contains("tank_policies") && replay["tank_policies"].is_object()) {
			for (auto it = replay["tank_policies"].begin(); it != replay["tank_policies"].end(); ++it) {
				if (!it.value().is_string()) continue;
				try {
					int tankId = std::stoi(it.key());
					tankPolicySpecs.emplace_back(tankId, it.value().get<std::string>());
				} catch (...) {
					// Ignore malformed tank id keys in replay JSON
				}
			}
		}
	}
	catch (const std::exception& ex) {
		std::cerr << "[GUI] Error: Invalid result JSON in " << resultPath << ": " << ex.what() << std::endl;
		return false;
	}

	return true;
}

static bool ParseTankPolicySpec(const std::string& spec, int& tankId, std::string& policyName) {
	auto sep = spec.find(':');
	if (sep == std::string::npos || sep == 0 || sep == spec.size() - 1) {
		return false;
	}

	try {
		tankId = std::stoi(spec.substr(0, sep));
	}
	catch (...) {
		return false;
	}

	policyName = spec.substr(sep + 1);
	return !policyName.empty();
}

std::vector<std::string> GetLevelByName(const std::string& name) {
    if (name == "level1") return GetLevel1();
    if (name == "level2") return GetLevel2();
    if (name == "level3") return GetLevel3();
    if (name == "level4") return GetLevel4();
    if (name == "level5") return GetLevel5();
    std::cerr << "Nivel desconocido: " << name << ". Usando level1 por defecto.\n";
    return GetLevel1();
}


int main(int argc, char* argv[]) {
	unsigned int seed = 42;
    int maxFrames = 1000;
    std::string levelName = "level1";
    std::string cheatsPath = "";
    int tickRate  = 10;  
	std::string resultPath = "";
	std::string teamAPolicy = "attack_base";
	std::string teamBPolicy = "attack_base";
	std::vector<std::pair<int, std::string>> tankPolicySpecs;

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg == "--result" && i + 1 < argc) resultPath = argv[++i];
	}

	if (!resultPath.empty() && !LoadReplayConfig(resultPath, seed, maxFrames, tickRate, levelName, cheatsPath, teamAPolicy, teamBPolicy, tankPolicySpecs)) {
		return 1;
	}

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--seed"      && i+1 < argc) seed       = std::stoul(argv[++i]);
        if (arg == "--maxFrames" && i+1 < argc) maxFrames  = std::stoi(argv[++i]);
        if (arg == "--level"     && i+1 < argc) levelName  = argv[++i];
        if (arg == "--cheats"    && i+1 < argc) cheatsPath = argv[++i];
        if (arg == "--tickRate"  && i+1 < argc) tickRate   = std::stoi(argv[++i]);
		if (arg == "--teamAPolicy" && i+1 < argc) teamAPolicy = argv[++i];
		if (arg == "--teamBPolicy" && i+1 < argc) teamBPolicy = argv[++i];
		if (arg == "--tankPolicy"  && i+1 < argc) {
			int tankId = -1;
			std::string policyName;
			if (!ParseTankPolicySpec(argv[++i], tankId, policyName)) {
				std::cerr << "[GUI] Invalid --tankPolicy format. Expected <tankId>:<agentType>." << std::endl;
				return 1;
			}
			tankPolicySpecs.emplace_back(tankId, policyName);
		}
    }

	tickRate = std::max(1, tickRate);
	maxFrames = std::max(1, maxFrames);

	std::cout << "[GUI] Replay config => seed=" << seed
			  << ", level=" << levelName
			  << ", tickRate=" << tickRate
			  << ", maxFrames=" << maxFrames
			  << ", teamAPolicy=" << teamAPolicy
			  << ", teamBPolicy=" << teamBPolicy
			  << ", cheats=" << (cheatsPath.empty() ? "(none)" : cheatsPath)
			  << std::endl;

	Runner runner;
	runner.MatchConfig(tickRate, maxFrames, seed);
	runner.SetLevelName(levelName);
	if (!runner.SetTeamPolicyByName('A', teamAPolicy)) {
		std::cerr << "[GUI] Invalid --teamAPolicy value: " << teamAPolicy << std::endl;
		return 1;
	}
	if (!runner.SetTeamPolicyByName('B', teamBPolicy)) {
		std::cerr << "[GUI] Invalid --teamBPolicy value: " << teamBPolicy << std::endl;
		return 1;
	}
	for (const auto& [tankId, policyName] : tankPolicySpecs) {
		if (!runner.SetTankPolicyByName(tankId, policyName)) {
			std::cerr << "[GUI] Invalid --tankPolicy agent type for tank " << tankId << ": " << policyName << std::endl;
			return 1;
		}
	}
	if (!cheatsPath.empty()) {
		runner.LoadCheatScript(cheatsPath);
	}
	runner.StartMatch(GetLevelByName(levelName));

	const GameState& game = runner.GetGameState();

	const int tileSize = 32;
	const int padding = 20;
	const int boardPixels = game.GetBoardSize() * tileSize;

	InitWindow(boardPixels + padding * 2, boardPixels + padding * 2 + 70, "BattleCity GUI (raylib)");
	SetTargetFPS(60);

	Render2D renderer(tileSize, padding);

	// Ticking: actualiza la lógica a una frecuencia fija para que sea jugable.
	const double tickSeconds = 1.0 / static_cast<double>(tickRate);
	double accumulator = 0.0;

	while (!WindowShouldClose()) {
		accumulator += GetFrameTime();

		// Lógica a ticks fijos
		while (accumulator >= tickSeconds) {
			if (!runner.GetGameState().IsGameOver()) {
				runner.StepMatch();
			}
			accumulator -= tickSeconds;
		}

		BeginDrawing();
		renderer.Draw(runner.GetGameState());
		EndDrawing();
	}

	CloseWindow();
	return 0;
}

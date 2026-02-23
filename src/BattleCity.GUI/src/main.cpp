#include <raylib.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include <algorithm>

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
	std::string& cheatsPath
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
	}
	catch (const std::exception& ex) {
		std::cerr << "[GUI] Error: Invalid result JSON in " << resultPath << ": " << ex.what() << std::endl;
		return false;
	}

	return true;
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

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg == "--result" && i + 1 < argc) resultPath = argv[++i];
	}

	if (!resultPath.empty() && !LoadReplayConfig(resultPath, seed, maxFrames, tickRate, levelName, cheatsPath)) {
		return 1;
	}

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--seed"      && i+1 < argc) seed       = std::stoul(argv[++i]);
        if (arg == "--maxFrames" && i+1 < argc) maxFrames  = std::stoi(argv[++i]);
        if (arg == "--level"     && i+1 < argc) levelName  = argv[++i];
        if (arg == "--cheats"    && i+1 < argc) cheatsPath = argv[++i];
        if (arg == "--tickRate"  && i+1 < argc) tickRate   = std::stoi(argv[++i]);
    }

	tickRate = std::max(1, tickRate);
	maxFrames = std::max(1, maxFrames);

	std::cout << "[GUI] Replay config => seed=" << seed
			  << ", level=" << levelName
			  << ", tickRate=" << tickRate
			  << ", maxFrames=" << maxFrames
			  << ", cheats=" << (cheatsPath.empty() ? "(none)" : cheatsPath)
			  << std::endl;

	Runner runner;
	runner.MatchConfig(tickRate, maxFrames, seed);
	runner.SetLevelName(levelName);
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

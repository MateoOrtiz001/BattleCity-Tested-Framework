#include <raylib.h>
#include <random>
#include <iostream>
#include <cstring>

#include "InputMap.h"
#include "Render2D.h"

#include "include/core/GameState.h"
#include "include/core/Action.h"
#include "include/ScriptedEnemyAgent.h"

// Layouts están en el Engine como función inline (archivo .cpp dentro de include/)
#include "include/map/Level1.cpp"

int main(int argc, char* argv[]) {
	// Parsear seed desde argumentos: --seed <valor>
	unsigned int seed = std::random_device{}();
	for (int i = 1; i < argc - 1; ++i) {
		if (std::strcmp(argv[i], "--seed") == 0) {
			seed = static_cast<unsigned int>(std::stoul(argv[i + 1]));
			break;
		}
	}
	std::cout << "[GUI] Using seed: " << seed << std::endl;

	GameState game;
	game.Initialize(GetLevel1());

	const int tileSize = 32;
	const int padding = 20;
	const int boardPixels = game.GetBoardSize() * tileSize;

	InitWindow(boardPixels + padding * 2, boardPixels + padding * 2 + 70, "BattleCity GUI (raylib)");
	SetTargetFPS(60);

	Render2D renderer(tileSize, padding);

	// Crear un agente por equipo con seeds deterministas
	ScriptedEnemyAgent agentA('A', ScriptedEnemyAgent::ScriptType::AttackBase, seed);
	ScriptedEnemyAgent agentB('B', ScriptedEnemyAgent::ScriptType::AttackBase, seed + 1);

	// Ticking: actualiza la lógica a una frecuencia fija para que sea jugable.
	const double tickSeconds = 1.0 / 10.0;
	double accumulator = 0.0;

	while (!WindowShouldClose()) {
		accumulator += GetFrameTime();

		// Lógica a ticks fijos
		while (accumulator >= tickSeconds) {
			// Equipo A decide acciones
			for (auto& tank : game.GetTeamATanks()) {
				if (!tank.IsAlive()) continue;

				Action action = agentA.Decide(game, tank.GetId());

				switch (action.type) {
					case ActionType::Move:
						game.MoveTank(tank, action.direction);
						break;
					case ActionType::Fire:
						game.TankShoot(tank);
						break;
					case ActionType::Stop:
					default:
						break;
				}
			}

			// Equipo B decide acciones
			for (auto& tank : game.GetTeamBTanks()) {
				if (!tank.IsAlive()) continue;

				Action action = agentB.Decide(game, tank.GetId());

				switch (action.type) {
					case ActionType::Move:
						game.MoveTank(tank, action.direction);
						break;
					case ActionType::Fire:
						game.TankShoot(tank);
						break;
					case ActionType::Stop:
					default:
						break;
				}
			}

			game.Update();
			accumulator -= tickSeconds;
		}

		BeginDrawing();
		renderer.Draw(game);
		EndDrawing();
	}

	CloseWindow();
	return 0;
}

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

	// Crear un agente con seed determinista para reproducibilidad
	ScriptedEnemyAgent enemyAgent(ScriptedEnemyAgent::ScriptType::AttackBase, seed);

	// Ticking: actualiza la lógica a una frecuencia fija para que sea jugable.
	const double tickSeconds = 1.0 / 10.0;
	double accumulator = 0.0;

	while (!WindowShouldClose()) {
		accumulator += GetFrameTime();

		// Input (solo controlamos el primer tanque del equipo A por ahora)
		InputAction action = InputMap::PollAction();
		if (!game.GetTeamATanks().empty()) {
			Tank& player = game.GetTeamATanks().front();
			if (action.moveDir != -1) {
				game.MoveTank(player, action.moveDir);
			}
			if (action.shoot) {
				game.TankShoot(player);
			}
		}

		// Lógica a ticks fijos
		while (accumulator >= tickSeconds) {
			// IA de enemigos: cada tick, cada tanque B decide una acción
			for (auto& enemy : game.GetTeamBTanks()) {
				if (!enemy.IsAlive()) continue;

				Action enemyAction = enemyAgent.Decide(game, enemy.GetId());

				switch (enemyAction.type) {
					case ActionType::Move:
						game.MoveTank(enemy, enemyAction.direction);
						break;
					case ActionType::Fire:
						game.TankShoot(enemy);
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

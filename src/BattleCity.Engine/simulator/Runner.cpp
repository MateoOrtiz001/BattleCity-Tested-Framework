#include "Runner.h"
#include "include/ScriptedEnemyAgent.h"
#include "external/json.hpp"
#include "fstream"

using json = nlohmann::json;

Runner::Runner() : seed(0), tickRate(10), maxFrames(500) {}

void Runner::MatchConfig(int ticks, int maxFrames, unsigned int seed) {
    this->tickRate = ticks;
    this->maxFrames = maxFrames;
    this->seed = seed;
}

void Runner::RunMatch(const vector<string>& layout) {
    gameState.Initialize(layout);

    // Crear agente con la seed configurada (determinista)
    ScriptedEnemyAgent enemyAgent(ScriptedEnemyAgent::ScriptType::AttackBase, seed);

    std::cout << "[Runner] Starting match with seed: " << seed << std::endl;

    while (!gameState.IsGameOver()) {
        // IA de enemigos: cada tick, cada tanque B decide una acción
        for (auto& enemy : gameState.GetTeamBTanks()) {
            if (!enemy.IsAlive()) continue;

            Action enemyAction = enemyAgent.Decide(gameState, enemy.GetId());

            switch (enemyAction.type) {
                case ActionType::Move:
                    gameState.MoveTank(enemy, enemyAction.direction);
                    break;
                case ActionType::Fire:
                    gameState.TankShoot(enemy);
                    break;
                case ActionType::Stop:
                default:
                    break;
            }
        }

        gameState.Update();
    }
}

void Runner::MatchResults(const std::string& filename) const {
    char winner = gameState.GetWinnerTeam();
    int frames = gameState.GetActualFrame();
    int score = gameState.GetScore();

    json result;

    result["seed"] = seed;
    result["frames"] = frames;
    result["score"] = score;
    result["winner"] = (winner == ' ') ? "Draw" : std::string(1, winner);

    std::ofstream file(filename);
    if (file.is_open()) {
        file << result.dump(4) << std::endl;
        file.close();
    } else {
        std::cerr << "[Runner] Error: No se pudo escribir el archivo del resultado" << std::endl;
    }
    
    std::cout << "[Runner] Match finished in " << frames << " frames" << std::endl;
    std::cout << "[Runner] Score: " << score << std::endl;
    if (winner == ' ')
        std::cout << "[Runner] Result: Draw" << std::endl;
    else
        std::cout << "[Runner] Winner: Team " << winner << std::endl;
}

unsigned int Runner::GetSeed() const {
    return seed;
}

const GameState& Runner::GetGameState() const {
    return gameState;
}
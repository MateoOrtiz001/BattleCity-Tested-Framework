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
    gameState.Initialize(layout, maxFrames);

    // Crear un agente por equipo, ambos con la misma seed base
    // Se usan seeds derivadas para que cada equipo tenga su propia secuencia
    ScriptedEnemyAgent agentA('A', ScriptedEnemyAgent::ScriptType::AttackBase, seed);
    ScriptedEnemyAgent agentB('B', ScriptedEnemyAgent::ScriptType::AttackBase, seed + 1);

    std::cout << "[Runner] Starting match with seed: " << seed << std::endl;

    while (!gameState.IsGameOver()) {
        // Equipo A decide acciones
        for (auto& tank : gameState.GetTeamATanks()) {
            if (!tank.IsAlive()) continue;

            Action action = agentA.Decide(gameState, tank.GetId());

            switch (action.type) {
                case ActionType::Move:
                    gameState.MoveTank(tank, action.direction);
                    break;
                case ActionType::Fire:
                    gameState.TankShoot(tank);
                    break;
                case ActionType::Stop:
                default:
                    break;
            }
        }

        // Equipo B decide acciones
        for (auto& tank : gameState.GetTeamBTanks()) {
            if (!tank.IsAlive()) continue;

            Action action = agentB.Decide(gameState, tank.GetId());

            switch (action.type) {
                case ActionType::Move:
                    gameState.MoveTank(tank, action.direction);
                    break;
                case ActionType::Fire:
                    gameState.TankShoot(tank);
                    break;
                case ActionType::Stop:
                default:
                    break;
            }
        }

        gameState.Update();
    }
}

void Runner::MatchResults(const std::string& filename, bool consoleLog) const {
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
        std::cerr << "[Runner] Error: Cannot write results file: " << filename << std::endl;
    }
    
    if (consoleLog) {
        std::cout << "[Runner] Match finished in " << frames << " frames" << std::endl;
        std::cout << "[Runner] Score: " << score << std::endl;
        if (winner == ' ')
            std::cout << "[Runner] Result: Draw" << std::endl;
        else
            std::cout << "[Runner] Winner: Team " << winner << std::endl;
    }
}

unsigned int Runner::GetSeed() const {
    return seed;
}

const GameState& Runner::GetGameState() const {
    return gameState;
}
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
    cheatManager = std::make_unique<CheatManager>(gameState);

    // Crear un agente por equipo, ambos con la misma seed base
    // Se usan seeds derivadas para que cada equipo tenga su propia secuencia
    ScriptedEnemyAgent agentA('A', ScriptedEnemyAgent::ScriptType::AttackBase, seed);
    ScriptedEnemyAgent agentB('B', ScriptedEnemyAgent::ScriptType::AttackBase, seed + 1);

    std::cout << "[Runner] Starting match with seed: " << seed << std::endl;

    while (!gameState.IsGameOver()) {

        ExecuteScheduledCheats(gameState.GetActualFrame());
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

void Runner::LoadCheatScript(const std::string& filePath){
    std::ifstream file(filePath);
    if(!file.is_open()){
        std::cerr << "[Runner] Error: Cannot open cheat script file: " << filePath << std::endl;
        return;
    }
    std::string line;
    while(std::getline(file, line)){
        if(line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        int frame;
        if (!(iss >> frame)) continue;
        std::string rest;
        std::getline(iss,rest);
        if(!rest.empty() && rest[0] == ' ') rest = rest.substr(1);
        scheduledCheats[frame].push_back(rest);
    }
    std::cout << "[Runner] Loaded cheat script: " << filePath << std::endl;

}

bool Runner::ExecuteCheat(const std::string& command){
    if(!cheatManager) return false;
    return cheatManager->ExecuteCommand(command);
}

void Runner::ExecuteScheduledCheats(int frame){
    auto it = scheduledCheats.find(frame);
    if (it != scheduledCheats.end()){
        for (const auto& cmd : it->second){
            ExecuteCheat(cmd);
        }
    }
}

GameState& Runner::GetMutableGameState(){
    return gameState;
}

CheatManager& Runner::GetCheatManager(){
    return *cheatManager;
}
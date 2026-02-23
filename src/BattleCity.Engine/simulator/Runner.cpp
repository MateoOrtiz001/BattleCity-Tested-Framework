#include "Runner.h"
#include "external/json.hpp"
#include "fstream"

using json = nlohmann::json;

Runner::Runner() : seed(0), tickRate(10), maxFrames(500) {}

void Runner::MatchConfig(int ticks, int maxFrames, unsigned int seed) {
    this->tickRate = ticks;
    this->maxFrames = maxFrames;
    this->seed = seed;
    scheduledCheats.clear();
    cheatScriptPath.clear();
}

void Runner::StartMatch(const vector<string>& layout) {
    agentMap.clear();
    gameState.Initialize(layout, maxFrames);
    cheatManager = std::make_unique<CheatManager>(gameState);

    std::cout << "[Runner] Starting match with seed: " << seed << std::endl;
}

void Runner::StepMatch() {
    if (gameState.IsGameOver()) {
        return;
    }

    ExecuteScheduledCheats(gameState.GetActualFrame());
    // Equipo A decide acciones
    for (auto& tank : gameState.GetTeamATanks()) {
        if (!tank.IsAlive()) continue;
        
        EnsureAgentExists(tank);
        Action action = agentMap[tank.GetId()]->Decide(gameState, tank.GetId());

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

        EnsureAgentExists(tank);
        Action action = agentMap[tank.GetId()]->Decide(gameState, tank.GetId());

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

void Runner::RunMatch(const vector<string>& layout) {
    StartMatch(layout);

    while (!gameState.IsGameOver()) {
        StepMatch();
    }
}

void Runner::MatchResults(const std::string& filename, bool consoleLog) const {
    char winner = gameState.GetWinnerTeam();
    int frames = gameState.GetActualFrame();
    int score = gameState.GetScore();

    json result;

    result["seed"] = seed;
    result["tick_rate"] = tickRate;
    result["max_frames"] = maxFrames;
    result["level"] = levelName;
    result["cheats_file"] = cheatScriptPath;
    result["frames"] = frames;
    result["score"] = score;
    result["winner"] = (winner == ' ') ? "Draw" : std::string(1, winner);

    if (cheatManager){
        const auto& cheatLog = cheatManager->GetLog();
        json cheatsArray = json::array();
        for (const auto& entry : cheatLog){
            cheatsArray.push_back({
                {"frame", entry.frame},
                {"command", entry.command},
                {"success", entry.success}
            });
        }
        result["cheats_executed"] = cheatsArray;
        result["cheats_total"] = static_cast<int>(cheatLog.size());
        result["cheats_failed"] = static_cast<int>(std::count_if(cheatLog.begin(),cheatLog.end(),
            [](const CheatLogEntry& e) { return !e.success; }));
    }

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
        if (cheatManager){
            std::cout << "[Runner] Cheats executed: " << cheatManager->GetLog().size() << std::endl;
        }
    }
}

void Runner::SetTeamPolicy(char team, ScriptedEnemyAgent::ScriptType policy){
    if (team == 'A'){
        teamAPolicy = policy;
    } else if (team == 'B'){
        teamBPolicy = policy;
    }
}

void Runner::EnsureAgentExists(const Tank& tank){
    int id = tank.GetId();
    if (agentMap.count(id) != 0) return;

    char team = tank.GetTeam();
    ScriptedEnemyAgent::ScriptType policy = (team == 'A') ? teamAPolicy : teamBPolicy;

    unsigned int tankSeed = seed ^ (static_cast<unsigned int>(id) * 2654435761u); // Mezcla la seed con el ID del tanque para diversidad
    agentMap[id] = std::make_unique<ScriptedEnemyAgent>(team, policy, tankSeed);
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
    cheatScriptPath = filePath;
    scheduledCheats.clear();
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

void Runner::SetLevelName(const std::string& levelName){
    this->levelName = levelName;
}
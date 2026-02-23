#include "include/core/CheatManager.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <utility>

using namespace std;

CheatManager::CheatManager(GameState& state, std::function<void(int, const std::string&)> onTankSpawnedWithPolicy)
    : gameState(state), onTankSpawnedWithPolicy(std::move(onTankSpawnedWithPolicy)) {
    RegisterCommands();
}

void CheatManager::RegisterCommands(){
    commands["spawn_tank"] = [this](const std::vector<std::string>& args) {
        // spawn_tank <x> <y> <team> <agent_type>
        if (args.size() < 4) throw invalid_argument("spawn_tank requires: <x> <y> <team> <agent_type>");
        auto maybeId = gameState.SpawnTank(stoi(args[0]), stoi(args[1]), args[2][0]);
        if (maybeId.has_value() && onTankSpawnedWithPolicy) {
            onTankSpawnedWithPolicy(*maybeId, args[3]);
        }
    };

    commands["spawn_tanks"] = [this](const std::vector<std::string>& args) {
        // spawn_tanks <count> <team> <agent_type>
        if (args.size() < 3) throw invalid_argument("spawn_tanks requires: <count> <team> <agent_type>");
        auto spawnedIds = gameState.SpawnTanks(stoi(args[0]), args[1][0]);
        if (onTankSpawnedWithPolicy) {
            for (int tankId : spawnedIds) {
                onTankSpawnedWithPolicy(tankId, args[2]);
            }
        }
    };

    commands["kill_tank"] = [this](const std::vector<std::string>& args) {
        // kill_tank <id>
        if (args.size() < 1) return;
        gameState.KillTank(stoi(args[0]));
    };

    commands["kill_all"] = [this](const std::vector<std::string>& args) {
        // kill_all <team>
        if (args.size() < 1) return;
        gameState.KillAllTanks(args[0][0]);
    };

    commands["heal_tank"] = [this](const std::vector<std::string>& args) {
        // heal_tank <id> <amount>
        if (args.size() < 2) return;
        gameState.HealTank(stoi(args[0]), stoi(args[1]));
    };

    commands["heal_all"] = [this](const std::vector<std::string>& args) {
        // heal_all <team> <amount>
        if (args.size() < 2) return;
        gameState.HealAllTanks(args[0][0], stoi(args[1]));
    };

    commands["set_lives"] = [this](const std::vector<std::string>& args) {
        // set_lives <tankId> <lives>
        if (args.size() < 2) return;
        gameState.SetTankLives(stoi(args[0]), stoi(args[1]));
    };

    commands["heal_base"] = [this](const std::vector<std::string>& args) {
        // heal_base <team> <amount>
        if (args.size() < 2) return;
        gameState.HealBase(args[0][0], stoi(args[1]));
    };

    commands["set_base_health"] = [this](const std::vector<std::string>& args) {
        // set_base_health <team> <health>
        if (args.size() < 2) return;
        gameState.SetBaseHealth(args[0][0], stoi(args[1]));
    };

    commands["destroy_base"] = [this](const std::vector<std::string>& args) {
        // destroy_base <team>
        if (args.size() < 1) return;
        gameState.DestroyBase(args[0][0]);
    };

    commands["wall_type"] = [this](const std::vector<std::string>& args) {
        // wall_type <x> <y> <type>
        if (args.size() < 3) return;
        gameState.ChangeWallType(stoi(args[0]), stoi(args[1]), args[2]);
    };

    commands["all_walls_type"] = [this](const std::vector<std::string>& args) {
        // all_walls_type <type>
        if (args.size() < 1) return;
        gameState.ChangeAllWallsType(args[0]);
    };

    commands["add_wall"] = [this](const std::vector<std::string>& args) {
        // add_wall <x> <y> <type>
        if (args.size() < 3) return;
        gameState.AddWall(stoi(args[0]), stoi(args[1]), args[2]);
    };

    commands["remove_wall"] = [this](const std::vector<std::string>& args) {
        // remove_wall <x> <y>
        if (args.size() < 2) return;
        gameState.RemoveWall(stoi(args[0]), stoi(args[1]));
    };

    commands["clear_walls"] = [this](const std::vector<std::string>& args) {
        gameState.ClearWalls();
    };

    commands["restart"] = [this](const std::vector<std::string>& args) {
        gameState.ForceRestart();
    };

    commands["game_over"] = [this](const std::vector<std::string>& args) {
        // game_over <winner: A|B|D>
        if (args.size() < 1) return;
        gameState.ForceGameOver(args[0][0] == 'D' ? ' ' : args[0][0]);
    };

    commands["set_tick_limit"] = [this](const std::vector<std::string>& args) {
        if (args.size() < 1) return;
        gameState.SetTickLimit(stoi(args[0]));
    };

    commands["set_score"] = [this](const std::vector<std::string>& args) {
        if (args.size() < 1) return;
        gameState.SetScore(stoi(args[0]));
    };

    commands["clear_bullets"] = [this](const std::vector<std::string>& args) {
        gameState.ClearAllBullets();
    };

    commands["spawn_bullet"] = [this](const std::vector<std::string>& args) {
        // spawn_bullet <x> <y> <direction> <team>
        if (args.size() < 4) return;
        gameState.SpawnBullet(stoi(args[0]), stoi(args[1]), stoi(args[2]), args[3][0]);
    };

    commands["pause"] = [this](const std::vector<std::string>& args) {
        gameState.PauseGame();
    };

    commands["resume"] = [this](const std::vector<std::string>& args) {
        gameState.ResumeGame();
    };

    commands["remove_tank"] = [this](const std::vector<std::string>& args) {
        if (args.size() < 1) return;
        gameState.RemoveTank(stoi(args[0]));
    };

    commands["remove_all_tanks"] = [this](const std::vector<std::string>& args) {
        if (args.size() < 1) return;
        gameState.RemoveAllTanks(args[0][0]);
    };
}

bool CheatManager::ExecuteCommand(const string& commandLine){
    auto tokens = Tokenize(commandLine);
    if (tokens.empty()) return false;

    string cmd = tokens[0];
    vector<string> args(tokens.begin() + 1, tokens.end());

    auto it = commands.find(cmd);
    bool succes = false;
    if (it != commands.end()){
        try {
            it->second(args);
            succes = true;
        } catch (const exception& e){
            cerr << "[Cheat] Error: " << e.what() << endl;
        }
    }
    log.push_back({gameState.GetActualFrame(), commandLine, succes});
    return succes;
}

int CheatManager::ProcessCommandFile(const string& filePath){
    ifstream file(filePath);
    if(!file.is_open()){
        cerr << "[Cheat] Error: Cannot open command file: " << filePath << endl;
        return -1;
    }

    int executed = 0;
    string line;
    while(getline(file,line)){
        if(line.empty() || line[0] == '#') continue;
        if (ExecuteCommand(line)){
            executed++;
        }
    }
    return executed;
}

vector<string> CheatManager::GetAvailableCommands() const {
    vector<string> result;
    for (const auto& [name,_] : commands){
        result.push_back(name);
    }
    return result;
}

vector<string> CheatManager::Tokenize(const string& line) const {
    vector<string> tokens;
    istringstream iss(line);
    string token;
    while(iss >> token){
        tokens.push_back(token);
    }
    return tokens;
}

const vector<CheatLogEntry>& CheatManager::GetLog() const {
    return log;
}

void CheatManager::ClearLog(){
    log.clear();
}
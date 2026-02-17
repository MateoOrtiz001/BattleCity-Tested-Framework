#pragma once
#include "../include/core/GameState.h"
#include "../include/core/Action.h"
#include "../include/core/CheatManager.h"
#include <vector>
#include <string>
#include <memory>
#include <random>
#include <iostream>

class IAgent;

class Runner{
    public:
        Runner();

        void RunMatch(const vector<string>& layout);
        void MatchConfig(int ticks, int maxFrames, unsigned int seed);
        void MatchResults(const std::string& filename, bool consoleLog=true) const;

        // Getters
        unsigned int GetSeed() const;
        const GameState& GetGameState() const;
        GameState& GetMutableGameState();
        CheatManager& GetCheatManager();

        // Cheats
        void LoadCheatScript(const std::string& filePath);
        bool ExecuteCheat(const std::string& command);

    private:
        unsigned int seed = 0;
        int tickRate = 10;
        int maxFrames = 500;
        GameState gameState;
        std::unique_ptr<CheatManager> cheatManager;
        std::unordered_map<int, std::vector<std::string>> scheduledCheats;
        void ExecuteScheduledCheats(int frame);
};
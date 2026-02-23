#pragma once
#include "../include/core/GameState.h"
#include "../include/core/Action.h"
#include "../include/core/CheatManager.h"
#include "include/agents/ScriptedEnemyAgent.h"
#include <vector>
#include <string>
#include <memory>
#include <random>
#include <iostream>
#include <algorithm>
#include <unordered_map>

using namespace std;


class Runner{
    public:
        Runner();

        void RunMatch(const vector<string>& layout);
        void StartMatch(const vector<string>& layout);
        void StepMatch();
        void MatchConfig(int ticks, int maxFrames, unsigned int seed);
        void MatchResults(const string& filename, bool consoleLog=true) const;
        void SetLevelName(const string& levelName);

        // Getters
        unsigned int GetSeed() const;
        const GameState& GetGameState() const;
        GameState& GetMutableGameState();
        CheatManager& GetCheatManager();
        void SetTeamPolicy(char team, ScriptedEnemyAgent::ScriptType policy);
        void SetTankPolicy(int tankId, ScriptedEnemyAgent::ScriptType policy);
        bool SetTeamPolicyByName(char team, const std::string& policyName);
        bool SetTankPolicyByName(int tankId, const std::string& policyName);

        // Cheats
        void LoadCheatScript(const string& filePath);
        bool ExecuteCheat(const string& command);

    private:
        unsigned int seed = 0;
        int tickRate = 10;
        int maxFrames = 500;
        string levelName = "level1";
        string cheatScriptPath = "";
        GameState gameState;
        unique_ptr<CheatManager> cheatManager;
        ScriptedEnemyAgent::ScriptType teamAPolicy = ScriptedEnemyAgent::ScriptType::AttackBase;
        ScriptedEnemyAgent::ScriptType teamBPolicy = ScriptedEnemyAgent::ScriptType::AttackBase;
        unordered_map<int, ScriptedEnemyAgent::ScriptType> tankPolicyMap;
        unordered_map<int, unique_ptr<IAgent>> agentMap;
        void EnsureAgentExists(const Tank& tank);
        unordered_map<int, vector<string>> scheduledCheats;
        void ExecuteScheduledCheats(int frame);
};
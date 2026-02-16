#pragma once
#include "../include/core/GameState.h"
#include "../include/core/Action.h"
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
        void MatchResults(const std::string& filename) const;

        // Getters
        unsigned int GetSeed() const;
        const GameState& GetGameState() const;

    private:
        unsigned int seed = 0;
        int tickRate = 10;
        int maxFrames = 500;
        GameState gameState;
};
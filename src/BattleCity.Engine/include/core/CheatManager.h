#pragma once
#include "GameState.h"

#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <functional>
#include <unordered_map>

using namespace std;

class CheatManager {
    public:
        explicit CheatManager(GameState& gameState);

        bool ExecuteCommand(const string& commandLine);
        int ProcessCommandFile(const string& filePath);

        vector<string> GetAvailableCommands() const;

    private:
        GameState& gameState;
        using CommandHandler = function<void(const vector<string>&)>;
        unordered_map<string, CommandHandler> commands;

        void RegisterCommands();
        vector<string> Tokenize(const string& line) const;

};
#pragma once
#include "GameState.h"

#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <functional>
#include <unordered_map>

using namespace std;

struct CheatLogEntry {
    int frame;
    string command;
    bool success;
};

class CheatManager {
    public:
        explicit CheatManager(GameState& gameState);

        bool ExecuteCommand(const string& commandLine);
        int ProcessCommandFile(const string& filePath);

        vector<string> GetAvailableCommands() const;

        const vector<CheatLogEntry>& GetLog() const;
        void ClearLog();

    private:
        GameState& gameState;
        using CommandHandler = function<void(const vector<string>&)>;
        unordered_map<string, CommandHandler> commands;
        vector<CheatLogEntry> log;

        void RegisterCommands();
        vector<string> Tokenize(const string& line) const;

};
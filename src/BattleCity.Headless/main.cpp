#include "simulator/Runner.h"
#include "include/map/Level1.cpp"
#include "include/map/Level2.cpp"
#include "include/map/Level3.cpp"
#include "include/map/Level4.cpp"
#include "include/map/Level5.cpp"
#include <iostream>
#include <cstring>
#include <random>

using namespace std;

int main(int argc, char* argv[]){
    unsigned int seed = random_device{}();
    int maxFrames = 500;
    int tickRate = 10;
    string outputFile = "results/result"+to_string(seed)+".json";
    string cheatFile = "";
    string level = "level1";

    for (int i = 1; i < argc; i++){
        if (strcmp(argv[i], "--seed") == 0){
            if (i + 1 >= argc) { cerr << "[Headless] Missing value for --seed" << endl; return 1; }
            seed = static_cast<unsigned int>(stoul(argv[i+1]));
            i++;
        }else if (strcmp(argv[i], "--maxFrames") == 0){
            if (i + 1 >= argc) { cerr << "[Headless] Missing value for --maxFrames" << endl; return 1; }
            maxFrames = stoi(argv[i+1]);
            i++;
        }else if (strcmp(argv[i], "--tickRate") == 0){
            if (i + 1 >= argc) { cerr << "[Headless] Missing value for --tickRate" << endl; return 1; }
            tickRate = stoi(argv[i+1]);
            i++;
        }else if (strcmp(argv[i], "--output") == 0){
            if (i + 1 >= argc) { cerr << "[Headless] Missing value for --output" << endl; return 1; }
            outputFile = argv[i+1];
            i++;
        }else if (strcmp(argv[i], "--cheats") == 0){
            if (i + 1 >= argc) { cerr << "[Headless] Missing value for --cheats" << endl; return 1; }
            cheatFile = argv[i+1];
            i++;
        }else if (strcmp(argv[i], "--level")== 0){
            if (i + 1 >= argc) { cerr << "[Headless] Missing value for --level" << endl; return 1; }
            level = argv[i+1];
            i++;
        }
    }
    cout << "[Headless] Starting a match" << endl;
    cout << "[Headless] Seed: " << seed << ", Max Frames: " << maxFrames << ", Tick Rate: " << tickRate << endl;

    Runner runner;
    runner.MatchConfig(tickRate, maxFrames, seed);
    if(!cheatFile.empty()){
        runner.LoadCheatScript(cheatFile);
        cout << "[HeadLess] Cheat script loaded: " << cheatFile << endl;
    }
    try {
        if (level == "level1") runner.RunMatch(GetLevel1());
        else if (level == "level2") runner.RunMatch(GetLevel2());
        else if (level == "level3") runner.RunMatch(GetLevel3());
        else if (level == "level4") runner.RunMatch(GetLevel4());
        else if (level == "level5") runner.RunMatch(GetLevel5());
        else {
            cerr << "[Headless] Error: " << level << " doesn't exists." << endl;
            return 1;
        }
        cout << "[Headless] Match finished. Saving results to " << outputFile << endl;
        runner.MatchResults(outputFile);
    } catch (const std::exception& e) {
        cerr << "[Headless] Error during match execution: " << e.what() << endl;
        return 1;
    }
    

    return 0;
}
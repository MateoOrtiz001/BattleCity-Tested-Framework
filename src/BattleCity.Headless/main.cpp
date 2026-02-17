#include "simulator/Runner.h"
#include "include/map/Level1.cpp"
#include <iostream>
#include <cstring>
#include <random>

using namespace std;

int main(int argc, char* argv[]){
    unsigned int seed = random_device{}();
    int maxFrames = 500;
    int tickRate = 10;
    string outputFile = "results/result"+to_string(seed)+".json";

    for (int i = 1; i < argc; i++){
        if (strcmp(argv[i], "--seed") == 0){
            seed = static_cast<unsigned int>(stoul(argv[i+1]));
        }else if (strcmp(argv[i], "--maxFrames") == 0){
            maxFrames = stoi(argv[i+1]);
        }else if (strcmp(argv[i], "--tickRate") == 0){
            tickRate = stoi(argv[i+1]);
        }else if (strcmp(argv[i], "--output") == 0){
            outputFile = argv[i+1];
        }
    }
    cout << "[Headless] Starting a match" << endl;
    cout << "[Headless] Seed: " << seed << ", Max Frames: " << maxFrames << ", Tick Rate: " << tickRate << endl;

    Runner runner;
    runner.MatchConfig(tickRate, maxFrames, seed);
    runner.RunMatch(GetLevel1());
    cout << "[Headless] Match finished. Saving results to " << outputFile << endl;
    runner.MatchResults(outputFile);

    return 0;
}
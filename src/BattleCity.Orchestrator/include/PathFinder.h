#pragma once

#include "include/core/GameState.h"
#include <vector>

struct PathResult{
    bool found = false;
    int firstDir = -1;
    bool firstStepIsWall = false;
    int totalCost = 0;
};

class PathFinder{
    public:
        static PathResult FindPath(
            const GameState& state,
            int startX, int startY,
            int goalX, int goalY,
            const Tank* ignoreTank = nullptr
        );
        
        struct Target {
            int x,y;
            int cost;
            bool isBase;
        };
        static Target FindCheapestTarget(
            const GameState& state,
            char myTeam,
            int startX, int startY,
            const Tank* ignoreTank = nullptr
        );
    
    
};
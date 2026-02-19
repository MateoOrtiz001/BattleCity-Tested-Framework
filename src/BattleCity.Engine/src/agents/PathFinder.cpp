#include "include/agents/PathFinder.h"
#include "include/utils/Utils.h"

#include <vector>
#include <queue>
#include <climits>

static const int PF_DX[] = { 0, 1, 0, -1};
static const int PF_DY[] = { 1, 0, -1, 0 };

PathResult PathFinder::FindPath(const GameState& state, int startX, int startY, int goalX, int goalY, const Tank* ignoreTank) {
    PathResult result;
    const int B = state.GetBoardSize();
    if (B <= 0) return result;

    if (startX == goalX && startY == goalY){
        result.found = true;
        result.firstDir = -1;
        result.totalCost = 0;
        return result;
    }

    auto idx = [&](int x, int y){return y * B + x;}; // índice lineal
    const int TOTAL = B * B;

    std::vector<int> gCost(TOTAL, INT_MAX);
    std::vector<int> parentIdx(TOTAL, -1);
    std::vector<int> parentDir(TOTAL, -1);

    struct Node {
        int f, g, x, y;
        bool operator>(const Node& o) const {return f > o.f;}
    };
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open;

    int sIdx = idx(startX, startY);
    gCost[sIdx] = 0;
    open.push({ManhattanDistance(startX, startY, goalX, goalY), 0, startX, startY});

    auto cellCost = [&](int nx, int ny)-> int {
        if (!state.IsValidPosition(nx, ny)) return -1;
        if (state.IsBlockedByTank(nx, ny, ignoreTank)) return -1;

        if (state.IsBlockedByWall(nx,ny)){
            for (const auto& wall : state.GetWalls()){
                if(wall->GetX() == nx && wall->GetY() == ny && wall->IsDestroyed()){
                    if(!wall->IsDestructible()) return -1;
                    return 1 + wall->GetHealth(); // Coste de destruir la pared
                }
            }
            return -1; // Pared indestructible
        }
        return 1; // Celda vacía, coste normal
    };

    int goalIdx = idx(goalX, goalY);
    bool found = false;

    while(!open.empty()){
        Node cur = open.top(); open.pop();
        int cIdx = idx(cur.x, cur.y);

        if (cIdx == goalIdx){found = true; break;}
        if (cur.g > gCost[cIdx]) continue; // Nodo obsoleto

        for (int dir = 0; dir < 4; dir++){
            int nx = cur.x + PF_DX[dir];
            int ny = cur.y + PF_DY[dir];
            int cost = cellCost(nx, ny);
            if (cost < 0) continue;

            int nIdx = idx(nx, ny);
            int newG = cur.g + cost;
            if (newG < gCost[nIdx]){
                gCost[nIdx] = newG;
                parentIdx[nIdx] = cIdx;
                parentDir[nIdx] = dir;
                int h = ManhattanDistance(nx, ny, goalX, goalY);
                open.push({newG + h, newG, nx, ny});
            }
        }
    }
    if (!found) return result;

    int cur = goalIdx;
    while(parentIdx[cur] != -1 && parentIdx[cur] != sIdx){
        cur = parentIdx[cur];
    }

    result.found = true;
    result.firstDir = parentDir[cur];
    result.totalCost = gCost[goalIdx];

    int nextX = startX + PF_DX[result.firstDir];
    int nextY = startY + PF_DY[result.firstDir];
    result.firstStepIsWall = state.IsBlockedByWall(nextX, nextY);
    return result;
}

PathFinder::Target PathFinder::FindCheapestTarget(
    const GameState& state,
    char myTeam,
    int startX, int startY,
    const Tank* ignoreTank
){
    char enemyTeam = (myTeam == 'A') ? 'B' : 'A';
    Target best{0,0,INT_MAX,true};

    const Base& enemyBase = state.GetBaseByTeam(enemyTeam);
    PathResult baseResult = FindPath(state, startX, startY, enemyBase.GetX(), enemyBase.GetY(), ignoreTank);
    if (baseResult.found){
        best = {enemyBase.GetX(), enemyBase.GetY(), baseResult.totalCost, true};
    }

    const auto& enemies = (enemyTeam == 'A') ? state.GetTeamATanks() : state.GetTeamBTanks();
    for (const auto& enemy : enemies){
        if (!enemy.IsAlive()) continue;
        PathResult r = FindPath(state, startX, startY, enemy.GetX(), enemy.GetY(), ignoreTank);
        if (r.found && r.totalCost < best.cost){
            best = {enemy.GetX(), enemy.GetY(), r.totalCost, false};
        }
    }
    return best;
}

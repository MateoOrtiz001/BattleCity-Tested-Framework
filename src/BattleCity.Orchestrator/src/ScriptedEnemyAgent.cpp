#include "include/ScriptedEnemyAgent.h"
#include "include/core/GameState.h"
#include "include/utils/Utils.h"
#include <vector>

// Deltas por dirección: 0=Norte, 1=Este, 2=Sur, 3=Oeste
static const int DX[] = { 0, 1, 0, -1 };
static const int DY[] = { 1, 0, -1, 0 };

ScriptedEnemyAgent::ScriptedEnemyAgent(char team, ScriptType type, unsigned int seed)
    : team(team)
    , scriptType(type)
    , rng(seed) {}

char ScriptedEnemyAgent::GetTeam() const {
    return team;
}

const std::vector<Tank>& ScriptedEnemyAgent::GetMyTanks(const GameState& state) const {
    return (team == 'A') ? state.GetTeamATanks() : state.GetTeamBTanks();
}

Action ScriptedEnemyAgent::Decide(const GameState& state, int tankId) {
    switch (scriptType) {
        case ScriptType::AttackBase: return AttackBase(state, tankId);
        case ScriptType::Random:    return RandomMove(state, tankId);
        case ScriptType::Defensive: return DefensiveAgent(state, tankId);
        case ScriptType::AStarAttack:     return AStarAttack(state, tankId);
        default:                    return Action::Stop();
    }
}

Action ScriptedEnemyAgent::AttackBase(const GameState& state, int tankId) {
    // Buscar el tanque por ID en el equipo de este agente
    const Tank* tank = nullptr;
    for (const auto& t : GetMyTanks(state)) {
        if (t.GetId() == tankId) { tank = &t; break; }
    }
    if (!tank || !tank->IsAlive()) return Action::Stop();

    // Objetivo: la base del equipo contrario
    char enemyTeam = (team == 'A') ? 'B' : 'A';
    const Base& targetBase = state.GetBaseByTeam(enemyTeam);
    int tankX = tank->GetX();
    int tankY = tank->GetY();
    int baseX = targetBase.GetX();
    int baseY = targetBase.GetY();

    int currentDist = ManhattanDistance(tankX, tankY, baseX, baseY);

    // Direcciones legales (no bloqueadas)
    std::vector<int> legalDirs = GetLegalDirections(state, tankId);

    if (legalDirs.empty()) return Action::Stop();

    // 1. Buscar la mejor dirección que acerque a la base
    int bestDir = -1;
    int minDist = currentDist;

    for (int dir : legalDirs) {
        int nx = tankX + DX[dir];
        int ny = tankY + DY[dir];
        int newDist = ManhattanDistance(nx, ny, baseX, baseY);
        if (newDist < minDist) {
            minDist = newDist;
            bestDir = dir;
        }
    }

    // 2. Decidir si disparar (60% de probabilidad si hay pared o tanque enfrente)
    {
        int frontX = tankX + DX[tank->GetDirection()];
        int frontY = tankY + DY[tank->GetDirection()];
        const Base& ownBase = state.GetBaseByTeam(team);
        bool targetAhead = state.IsBlockedByWall(frontX, frontY)
                        || state.IsBlockedByTank(frontX, frontY, tank);

        bool enemySight = EnemyInSight(state, tank);
        double fireChance = enemySight ? 0.9 : (targetAhead ? 0.6 : 0.15);
        std::uniform_real_distribution<double> prob(0.0, 1.0);
        if (!AimsAtBase(state, tank, ownBase.GetX(), ownBase.GetY()) && prob(rng) < fireChance) {
            return Action::Fire();
        }
    }

    // 3. Si encontramos dirección que acerca, 70% de tomarla
    if (bestDir != -1) {
        std::uniform_real_distribution<double> prob(0.0, 1.0);
        if (prob(rng) < 0.7) {
            return Action::Move(bestDir);
        }
    }

    // 4. Fallback: dirección aleatoria de las legales
    std::uniform_int_distribution<int> dist(0, static_cast<int>(legalDirs.size()) - 1);
    return Action::Move(legalDirs[dist(rng)]);
}

Action ScriptedEnemyAgent::RandomMove(const GameState& state, int tankId) {
    std::vector<int> legalDirs = GetLegalDirections(state, tankId);
    if (legalDirs.empty()) return Action::Stop();

    // 20% de disparar
    std::uniform_real_distribution<double> prob(0.0, 1.0);
    if (prob(rng) < 0.2) return Action::Fire();

    std::uniform_int_distribution<int> dist(0, static_cast<int>(legalDirs.size()) - 1);
    return Action::Move(legalDirs[dist(rng)]);
}

Action ScriptedEnemyAgent::DefensiveAgent(const GameState& state, int tankId){
    const Tank* tank = nullptr;
    for (const auto& t : GetMyTanks(state)){
        if (t.GetId() == tankId) {tank = &t; break;}
    }
    if(!tank || !tank->IsAlive()) return Action::Stop();
    const Base& targetBase = state.GetBaseByTeam(team);
    int tankX = tank->GetX();
    int tankY = tank->GetY();
    int baseX = targetBase.GetX();
    int baseY = targetBase.GetY();
    int distBase = ManhattanDistance(tankX, tankY, baseX, baseY);

    std::vector<int> legalDirs = GetLegalDirections(state, tankId);
    if(legalDirs.empty()) return Action::Stop();

    int bestDir = -1;
    int minDist = distBase;
    for (int dir : legalDirs) {
        int nx = tankX + DX[dir];
        int ny = tankY + DY[dir];
        int newDist = ManhattanDistance(nx, ny, baseX, baseY);
        if (newDist < minDist) {
            minDist = newDist;
            bestDir = dir;
        }
    }
    {
        int frontX = tankX + DX[tank->GetDirection()];
        int frontY = tankY + DY[tank->GetDirection()];
        bool enemySight = EnemyInSight(state, tank);

        if(enemySight){
            return Action::Fire();
        }
    }

    if(bestDir != -1){
        std::uniform_real_distribution<double> prob(0.0, 1.0);
        if (prob(rng) < 0.6 && minDist > 3){
            return Action::Move(bestDir);
        }else{
            std::uniform_int_distribution<int> dist(0, static_cast<int>(legalDirs.size()) - 1);
            return Action::Move(legalDirs[dist(rng)]);
        }
    }

}

Action ScriptedEnemyAgent::AStarAttack(const GameState& state, int tankId){
    const Tank* tank = nullptr;
    for (const auto& t : GetMyTanks(state)){
        if (t.GetId() == tankId) {tank = &t; break;}
    }
    if( !tank || !tank->IsAlive()) return Action::Stop();

    PathFinder::Target target = PathFinder::FindCheapestTarget(
        state, team, tank->GetX(), tank->GetY(), tank
    );

    if(!target.cost || target.cost == INT_MAX){
        std::vector<int> legal = GetLegalDirections(state, tankId);
        if (legal.empty()) return Action::Stop();
        std::uniform_int_distribution<int> dist(0, (int)legal.size() - 1);
        return Action::Move(legal[dist(rng)]);
    }
    PathResult path = PathFinder::FindPath(
        state, tank->GetX(), tank->GetY(), target.x, target.y, tank
    );

    if (!path.found) return Action::Stop();

    if(path.firstDir == -1) return Action::Fire();

    if (path.firstStepIsWall){
        if(tank->GetDirection() == path.firstDir) return Action::Fire();
        return Action::Move(path.firstDir);
    }
    if (EnemyInSight(state, tank)) return Action::Fire();

    return Action::Move(path.firstDir);
}

std::vector<int> ScriptedEnemyAgent::GetLegalDirections(const GameState& state, int tankId) {
    const Tank* tank = nullptr;
    for (const auto& t : GetMyTanks(state)) {
        if (t.GetId() == tankId) { tank = &t; break; }
    }
    if (!tank || !tank->IsAlive()) return {};

    std::vector<int> dirs;
    dirs.reserve(4);

    for (int dir = 0; dir < 4; ++dir) {
        int nx = tank->GetX() + DX[dir];
        int ny = tank->GetY() + DY[dir];
        if (state.IsValidPosition(nx, ny) && !state.IsBlockedByWall(nx, ny)
            && !state.IsBlockedByTank(nx, ny, tank)) {
            dirs.push_back(dir);
        }
    }
    return dirs;
}

bool ScriptedEnemyAgent::AimsAtBase(const GameState& state, const Tank* tank, int baseX, int baseY) const{
    int dir = tank->GetDirection();
    int cx = tank->GetX() + DX[dir];
    int cy = tank->GetY() + DY[dir];
    while (state.IsValidPosition(cx,cy)){
        if (cx == baseX && cy == baseY) return true;
        cx += DX[dir];
        cy += DY[dir];
    }
    return false;
}

bool ScriptedEnemyAgent::EnemyInSight(const GameState& state, const Tank* tank) const{
    char enemyTeam = (team == 'A') ? 'B' : 'A';
    const auto& enemies = (enemyTeam == 'A') ? state.GetTeamATanks() : state.GetTeamBTanks();

    int dir = tank->GetDirection();
    int cx = tank->GetX() + DX[dir];
    int cy = tank->GetY() + DY[dir];

    while (state.IsValidPosition(cx, cy) && !state.IsBlockedByWall(cx, cy)) {
        for (const auto& enemy : enemies){
            if (enemy.IsAlive() && enemy.GetX() == cx && enemy.GetY() == cy){
                return true;
            }
        }
        cx += DX[dir];
        cy += DY[dir];
    }
    return false;
}
#include "include/ScriptedEnemyAgent.h"
#include "include/core/GameState.h"
#include "include/utils/Utils.h"
#include <vector>

// Deltas por dirección: 0=Norte, 1=Este, 2=Sur, 3=Oeste
static const int DX[] = { 0, 1, 0, -1 };
static const int DY[] = { 1, 0, -1, 0 };

ScriptedEnemyAgent::ScriptedEnemyAgent(ScriptType type, unsigned int seed)
    : scriptType(type)
    , rng(seed) {}
    // Nota: seed=0 es un valor válido y determinista.
    // Para obtener una seed aleatoria, usar std::random_device{}() antes de pasar el valor.

Action ScriptedEnemyAgent::Decide(const GameState& state, int tankId) {
    switch (scriptType) {
        case ScriptType::AttackBase: return AttackBase(state, tankId);
        case ScriptType::Random:    return RandomMove(state, tankId);
        default:                    return Action::Stop();
    }
}

Action ScriptedEnemyAgent::AttackBase(const GameState& state, int tankId) {
    // Buscar el tanque por ID
    const Tank* tank = nullptr;
    for (const auto& t : state.GetTeamBTanks()) {
        if (t.GetId() == tankId) { tank = &t; break; }
    }
    if (!tank || !tank->IsAlive()) return Action::Stop();

    // Objetivo: la base del equipo contrario (A)
    const Base& targetBase = state.GetBaseByTeam('A');
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
        bool targetAhead = state.IsBlockedByWall(frontX, frontY)
                        || state.IsBlockedByTank(frontX, frontY, tank);

        std::uniform_real_distribution<double> prob(0.0, 1.0);
        double fireChance = targetAhead ? 0.6 : 0.15;
        if (prob(rng) < fireChance) {
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

std::vector<int> ScriptedEnemyAgent::GetLegalDirections(const GameState& state, int tankId) {
    const Tank* tank = nullptr;
    for (const auto& t : state.GetTeamBTanks()) {
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

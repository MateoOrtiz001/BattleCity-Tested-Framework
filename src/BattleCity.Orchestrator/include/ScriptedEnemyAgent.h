#pragma once

#include "IAgent.h"
#include "include/core/GameState.h"
#include <random>
#include <string>
#include <vector>

class ScriptedEnemyAgent : public IAgent {
public:
    enum class ScriptType {
        AttackBase,
        Random
    };

    /// @param team Equipo al que pertenece este agente ('A' o 'B').
    /// @param seed Semilla para el generador aleatorio. Usar la misma seed
    ///             produce la misma secuencia de decisiones (reproducibilidad).
    ScriptedEnemyAgent(char team, ScriptType type = ScriptType::AttackBase, unsigned int seed = 0);

    Action Decide(const GameState& state, int tankId) override;

    char GetTeam() const;

private:
    Action AttackBase(const GameState& state, int tankId);
    Action RandomMove(const GameState& state, int tankId);

    // Devuelve las direcciones en las que el tanque puede moverse (no bloqueado)
    std::vector<int> GetLegalDirections(const GameState& state, int tankId);

    // Obtiene la lista de tanques del equipo de este agente
    const std::vector<Tank>& GetMyTanks(const GameState& state) const;

    char team;
    ScriptType scriptType;
    std::mt19937 rng;
};

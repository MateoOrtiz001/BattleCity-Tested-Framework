#pragma once

#include "IAgent.h"
#include <random>
#include <string>

class ScriptedEnemyAgent : public IAgent {
public:
    enum class ScriptType {
        AttackBase,
        Random
    };

    /// @param seed Semilla para el generador aleatorio. Usar la misma seed
    ///             produce la misma secuencia de decisiones (reproducibilidad).
    ScriptedEnemyAgent(ScriptType type = ScriptType::AttackBase, unsigned int seed = 0);

    Action Decide(const GameState& state, int tankId) override;

private:
    Action AttackBase(const GameState& state, int tankId);
    Action RandomMove(const GameState& state, int tankId);

    // Devuelve las direcciones en las que el tanque puede moverse (no bloqueado)
    std::vector<int> GetLegalDirections(const GameState& state, int tankId);

    ScriptType scriptType;
    std::mt19937 rng;
};

#pragma once

#include "include/core/Action.h"

class GameState;

// Interfaz base para cualquier agente (bot, jugador, IA)
class IAgent {
public:
    virtual ~IAgent() = default;
    virtual Action Decide(const GameState& state, int tankId) = 0;
};

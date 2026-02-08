#pragma once

// Tipos de acciones que un tanque puede realizar
enum class ActionType {
    Stop,
    Move,
    Fire
};

// Representa una acción completa para un tanque
struct Action {
    ActionType type = ActionType::Stop;
    int direction = -1; // 0=Norte, 1=Este, 2=Sur, 3=Oeste (solo relevante para Move)

    static Action Stop() { return { ActionType::Stop, -1 }; }
    static Action Move(int dir) { return { ActionType::Move, dir }; }
    static Action Fire() { return { ActionType::Fire, -1 }; }
};

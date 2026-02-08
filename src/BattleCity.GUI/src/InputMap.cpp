#include "InputMap.h"

#include <raylib.h>

InputAction InputMap::PollAction() {
    InputAction action;

    // Discrete movement (one tile per key press)
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) action.moveDir = 0;
    if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) action.moveDir = 1;
    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) action.moveDir = 2;
    if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)) action.moveDir = 3;

    action.shoot = IsKeyPressed(KEY_SPACE);

    return action;
}

#include "Render2D.h"

#include <raylib.h>

#include "include/core/GameState.h"

Render2D::Render2D(int tileSize, int padding)
    : tileSize(tileSize), padding(padding) {}

void Render2D::Draw(const GameState& state) const {
    const int boardSize = state.GetBoardSize();
    const int originX = padding;
    const int originY = padding;

    // Background
    ClearBackground(RAYWHITE);

    // Grid (light)
    for (int y = 0; y <= boardSize; ++y) {
        const int py = originY + (boardSize - y) * tileSize;
        DrawLine(originX, py, originX + boardSize * tileSize, py, LIGHTGRAY);
    }
    for (int x = 0; x <= boardSize; ++x) {
        const int px = originX + x * tileSize;
        DrawLine(px, originY, px, originY + boardSize * tileSize, LIGHTGRAY);
    }

    // Walls
    for (const auto& wallPtr : state.GetWalls()) {
        const int wx = wallPtr->GetX();
        const int wy = wallPtr->GetY();
        const int px = originX + wx * tileSize;
        const int py = originY + (boardSize - 1 - wy) * tileSize;

        const Color color = wallPtr->IsDestructible() ? ORANGE : DARKGRAY;
        DrawRectangle(px + 1, py + 1, tileSize - 2, tileSize - 2, color);
    }

    // Bases
    {
        const Base& baseA = state.GetBaseByTeam('A');
        DrawCell(baseA.GetX(), baseA.GetY(), boardSize, originX, originY, GREEN);
    }
    {
        const Base& baseB = state.GetBaseByTeam('B');
        DrawCell(baseB.GetX(), baseB.GetY(), boardSize, originX, originY, MAROON);
    }

    // Tanks
    for (const auto& tank : state.GetTeamATanks()) {
        if (!tank.IsAlive()) continue;
        DrawCell(tank.GetX(), tank.GetY(), boardSize, originX, originY, BLUE);
    }
    for (const auto& tank : state.GetTeamBTanks()) {
        if (!tank.IsAlive()) continue;
        DrawCell(tank.GetX(), tank.GetY(), boardSize, originX, originY, RED);
    }

    // Bullets
    for (const auto& bullet : state.GetBullets()) {
        const int bx = bullet.GetX();
        const int by = bullet.GetY();
        const int px = originX + bx * tileSize + tileSize / 2;
        const int py = originY + (boardSize - 1 - by) * tileSize + tileSize / 2;
        DrawCircle(px, py, tileSize * 0.18f, BLACK);
    }

    // HUD
    DrawText(TextFormat("Frame: %d", state.GetActualFrame()), originX, originY + boardSize * tileSize + 10, 18, DARKGRAY);
    DrawText(TextFormat("Score: %d", state.GetScore()), originX + 140, originY + boardSize * tileSize + 10, 18, DARKGRAY);

    if (state.IsGameOver()) {
        const char winner = state.GetWinnerTeam();
        const char* msg = (winner == 'A') ? "Game Over - Winner: A" : (winner == 'B') ? "Game Over - Winner: B" : "Game Over - Draw";
        DrawText(msg, originX, originY + boardSize * tileSize + 36, 22, BLACK);
    }
}

void Render2D::DrawCell(int x, int y, int boardSize, int originX, int originY, Color color) const {
    const int px = originX + x * tileSize;
    const int py = originY + (boardSize - 1 - y) * tileSize;
    DrawRectangle(px + 2, py + 2, tileSize - 4, tileSize - 4, color);
}

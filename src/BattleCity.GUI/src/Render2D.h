#pragma once

// Needed for Color type in method signatures
#include <raylib.h>

class GameState;

class Render2D {
	public:
		Render2D(int tileSize, int padding);
		void Draw(const GameState& state) const;

	private:
		void DrawCell(int x, int y, int boardSize, int originX, int originY, Color color) const;

		int tileSize;
		int padding;
};

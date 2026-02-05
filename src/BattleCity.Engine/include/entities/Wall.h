#pragma once

#include <string>

class Wall {
    public:
        Wall() = default;
        Wall(int x, int y, const std::string& wallType);

        int GetX() const;
        int GetY() const;
        bool IsDestructible() const;
        bool IsDestroyed() const;
        void TakeDamage(int damage);

    private:
        int x, y; 
        bool destructible = true; 
        int health = 1;
};
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
        int GetHealth() const;
        std::string GetType() const;

        void SetType(const std::string& wallType);
        void SetHealth(int h);
        void TakeDamage(int damage);

    private:
        int x, y; 
        bool destructible = true;
        int health = 3;
};
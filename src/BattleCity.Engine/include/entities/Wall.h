#pragma once

class Wall {
    public:
        int GetX() const;
        int GetY() const;
        bool IsDestructible() const;
        void TakeDamage(int damage);

    private:
        int x, y; 
        bool destructible; 
        int health;
};
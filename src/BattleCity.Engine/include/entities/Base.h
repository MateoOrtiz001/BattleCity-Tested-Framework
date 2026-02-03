#pragma once

class Base {
    public:
        int GetX() const;
        int GetY() const;
        bool IsAlive() const;
        void TakeDamage(int damage);
    
    private:
        int x, y; 
        int health = 1;
};
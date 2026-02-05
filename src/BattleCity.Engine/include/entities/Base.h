#pragma once

class Base {
    public:
        Base() = default;
        Base(int x, int y, char team);

        int GetX() const;
        int GetY() const;
        bool IsAlive() const;
        void TakeDamage(int damage, char team);
    
    private:
        int x = 0, y = 0; 
        int health = 1;
        char team;
};
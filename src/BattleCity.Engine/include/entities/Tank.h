#pragma once

class Tank {
    public:
        int GetX() const;
        int GetY() const;
        int GetDirection() const;
        int GetHealth() const;
        int GetId() const;
        void Move(int deltaX, int deltaY);
        void SetDirection(int newDirection);
        void Respawn();
        void TakeDamage(int damage);

    private:
        int x, y; 
        int spawnX, spawnY; 
        int direction; 
        int health;
        int id;
};
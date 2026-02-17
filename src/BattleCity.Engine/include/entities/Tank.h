#pragma once

class Tank {
    public:
        Tank() = default;
        Tank(int x, int y, char team);

        int GetX() const;
        int GetY() const;
        int GetDirection() const;
        int GetHealth() const;
        int GetLives() const;
        int GetId() const;
        char GetTeam() const;
        bool IsAlive() const;
        int GetSpawnX() const;
        int GetSpawnY() const;
        int GetMaxHealth() const;

        
        void SetPosition(int newX, int newY);
        void Move(int deltaX, int deltaY);
        void SetDirection(int newDirection);
        void Respawn();
        void TakeDamage(int damage);
        void SetHealth(int h);
        void Heal(int amount);
        void SetLives(int l);
        void Kill();
        void FullHeal();


        // Resetear el contador de IDs para partidas reproducibles
        static void ResetIdCounter();

    private:
        static int nextId;
        int x, y; 
        int spawnX, spawnY; 
        int direction = 0; 
        int health = 3;
        int id;
        char team;
        int lives = 5;
};
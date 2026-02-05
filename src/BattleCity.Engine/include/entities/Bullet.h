#pragma once

class Bullet {
    public:
        Bullet() = default;
        Bullet(int x, int y, int direction, char team);

        int GetX() const;
        int GetY() const;
        int GetPreviousX() const;
        int GetPreviousY() const;
        int GetDirection() const;
        char GetTeam() const;
        bool IsActive() const;
        void Move();
        void Deactivate();

    private:
        int x, y; 
        int direction; 
        char team;
        int previousX = 0, previousY = 0;
        bool active = true;
};
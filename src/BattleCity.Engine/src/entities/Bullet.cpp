#include "include/entities/Bullet.h"

Bullet::Bullet(int x, int y, int direction, char team) 
    : x(x), y(y), direction(direction), team(team), previousX(x), previousY(y), active(true) {}


int Bullet::GetX() const {
    return x;
}

int Bullet::GetY() const {
    return y;
}

int Bullet::GetPreviousX() const {
    return previousX;
}

int Bullet::GetPreviousY() const {
    return previousY;
}

int Bullet::GetDirection() const {
    return direction;
}

char Bullet::GetTeam() const {
    return team;
}

bool Bullet::IsActive() const {
    return active;
}

void Bullet::Deactivate() {
    active = false;
}

void Bullet::Move() {
    previousX = x;
    previousY = y;
    
    int dx = 0, dy = 0;
    switch (direction) {
    case 0: dy = 1; break;   // Norte
    case 1: dx = 1; break;   // Este
    case 2: dy = -1; break;  // Sur
    case 3: dx = -1; break;  // Oeste
    default: break;
    }
    x += dx;
    y += dy;
}
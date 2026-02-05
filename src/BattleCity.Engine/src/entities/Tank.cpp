#include "include/entities/Tank.h"

int Tank::nextId = 0;

Tank::Tank(int x, int y, char team) 
    : x(x), y(y), spawnX(x), spawnY(y), team(team), direction(0), health(3), id(nextId++) {}

int Tank::GetX() const {
    return x;
}

int Tank::GetY() const {
    return y;
}

int Tank::GetDirection() const {
    return direction;
}

int Tank::GetHealth() const {
    return health;
}

int Tank::GetLives() const {
    return lives;
}

char Tank::GetTeam() const {
    return team;
}

int Tank::GetId() const {
    return id;
}

void Tank::Move(int deltaX, int deltaY) {
    x += deltaX;
    y += deltaY;
}

void Tank::SetDirection(int newDirection) {
    direction = newDirection;
}

void Tank::Respawn() {
    x = spawnX;
    y = spawnY;
    health = 3; // Reset health on respawn
}

void Tank::TakeDamage(int damage) {
    health -= damage;
    if (health == 0){
        lives--;
    }
}   

bool Tank::IsAlive() const {
    return lives > 0;
}

void Tank::SetPosition(int newX, int newY) {
    x = newX;
    y = newY;
}
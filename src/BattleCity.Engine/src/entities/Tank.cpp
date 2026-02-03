#include "include/entities/Tank.h"

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
    if (health < 0) {
        health = 0;
    }
}
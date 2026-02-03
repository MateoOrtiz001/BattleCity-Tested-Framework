#include "include/entities/Wall.h"

int Wall::GetX() const {
    return x;
}

int Wall::GetY() const {
    return y;
}

bool Wall::IsDestructible() const {
    return destructible;
}

void Wall::TakeDamage(int damage) {
    if (!destructible) {
        return;
    }
    health -= damage;
    if (health < 0) {
        health = 0;
    }
}
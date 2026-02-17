#include "include/entities/Wall.h"

Wall::Wall(int x, int y, const std::string& wallType) 
    : x(x), y(y) {
    if (wallType == "steel") {
        destructible = false;
        health = 999;
    } else {
        destructible = true;
        health = 1;
    }
}

int Wall::GetX() const {
    return x;
}

int Wall::GetY() const {
    return y;
}

bool Wall::IsDestructible() const {
    return destructible;
}

bool Wall::IsDestroyed() const {
    return health <= 0;
}

int Wall::GetHealth() const {
    return health;
}

std::string Wall::GetType() const {
    return destructible ? "brick" : "steel";
}

void Wall::SetType(const std::string& wallType){
    if (wallType == "steel") {
        destructible = false;
        health = 999;
    } else {
        destructible = true;
        health = 1;
    }
}

void Wall::SetHealth(int h) {
    health = h;
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
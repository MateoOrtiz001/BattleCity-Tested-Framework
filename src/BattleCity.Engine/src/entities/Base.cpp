#include "include/entities/Base.h"

Base::Base(int x, int y, char team) : x(x), y(y), health(1), team(team) {}

int Base::GetX() const {
    return x;
}

int Base::GetY() const {
    return y;
}
bool Base::IsAlive() const {
    return health > 0;
}

void Base::TakeDamage(int damage, char team) {
    if (this->team == team) {
        return; // No se recibe daño de la misma equipo
    }
    health -= damage;
    if (health < 0) {
        health = 0;
    }
}


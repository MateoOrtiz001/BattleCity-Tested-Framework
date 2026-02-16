#include "include/core/GameState.h"
#include <algorithm>

//// INICIALIZACIÓN Y CONTROL ////

void GameState::Initialize(const vector<string>& layout) {
    // Guardar layout original para reset
    originalLayout = layout;
    
    // Resetear contador de IDs para reproducibilidad
    Tank::ResetIdCounter();
    
    // Limpiar estado previo
    teamA_tanks.clear();
    teamB_tanks.clear();
    walls.clear();
    bullets.clear();
    gameOver = false;
    winnerTeam = ' ';
    score = 0;
    actualFrame = 0;

    // Ajustar el tamaño del tablero en base al layout proporcionado
    int height = static_cast<int>(layout.size());
    boardSize = height;

    for (int y = 0; y < static_cast<int>(layout.size()); y++) {
        for (int x = 0; x < static_cast<int>(layout[y].size()); x++) {
            char cell = layout[y][x];
            // Invertir coordenada Y para que (0,0) esté en la esquina inferior izquierda
            int posX = x;
            int posY = static_cast<int>(layout.size()) - 1 - y;

            if (cell == 'A') {
                Tank tank(posX, posY, 'A');
                teamA_tanks.push_back(tank);
            } else if (cell == 'B') {
                Tank tank(posX, posY, 'B');
                teamB_tanks.push_back(tank);
            } else if (cell == 'a') {
                baseA = Base(posX, posY, 'A');
            } else if (cell == 'b') {
                baseB = Base(posX, posY, 'B');
            } else if (cell == 'X') {
                auto wall = make_shared<Wall>(posX, posY, "brick");
                walls.push_back(wall);
            } else if (cell == 'S') {
                auto wall = make_shared<Wall>(posX, posY, "steel");
                walls.push_back(wall);
            }
        }
    }
}

void GameState::Update() {
    if (gameOver) return;

    actualFrame++;
    
    UpdateBullets();
    CheckBulletCollisions();
    RemoveDestroyedWalls();
    CheckGameOver();
    
    if (actualFrame >= timelimit) {
        gameOver = true;
        winnerTeam = ' '; // Empate por tiempo
    }
}

void GameState::Reset() {
    Initialize(originalLayout);
}

bool GameState::IsGameOver() const {
    return gameOver;
}

//// ACCIONES DE TANQUES ////

bool GameState::MoveTank(Tank& tank, int direction) {
    if (!tank.IsAlive()) return false;
    
    // Siempre actualizar dirección
    tank.SetDirection(direction);
    
    // Calcular nueva posición
    int dx = 0, dy = 0;
    switch (direction) {
        case 0: dy = 1; break;   // Norte
        case 1: dx = 1; break;   // Este
        case 2: dy = -1; break;  // Sur
        case 3: dx = -1; break;  // Oeste
    }
    
    int newX = tank.GetX() + dx;
    int newY = tank.GetY() + dy;
    
    // Verificar si puede moverse
    if (!IsValidPosition(newX, newY)) return false;
    if (IsBlockedByWall(newX, newY)) return false;
    if (IsBlockedByTank(newX, newY, &tank)) return false;
    
    tank.SetPosition(newX, newY);
    return true;
}

void GameState::TankShoot(Tank& tank) {
    if (!tank.IsAlive()) return;
    
    // Crear bala en la posición del tanque, en su dirección
    int bulletX = tank.GetX();
    int bulletY = tank.GetY();
    
    // Mover bala una posición adelante del tanque
    switch (tank.GetDirection()) {
        case 0: bulletY++; break;
        case 1: bulletX++; break;
        case 2: bulletY--; break;
        case 3: bulletX--; break;
    }
    
    // Verificar si la posición de spawn está fuera del mapa
    if (!IsValidPosition(bulletX, bulletY)) {
        return;
    }
    
    // Colisión inmediata con paredes en la posición de spawn
    for (auto& wall : walls) {
        if (wall->GetX() == bulletX && wall->GetY() == bulletY) {
            wall->TakeDamage(1);
            return;
        }
    }
    
    // Colisión inmediata con tanques enemigos
    for (auto& t : teamA_tanks) {
        if (t.IsAlive() && t.GetX() == bulletX && t.GetY() == bulletY && tank.GetTeam() != 'A') {
            t.TakeDamage(1);
            if (t.GetHealth() == 0 && t.GetLives() > 0) {
                t.Respawn();
            }
            score += 100;
            return;
        }
    }
    for (auto& t : teamB_tanks) {
        if (t.IsAlive() && t.GetX() == bulletX && t.GetY() == bulletY && tank.GetTeam() != 'B') {
            t.TakeDamage(1);
            if (t.GetHealth() == 0 && t.GetLives() > 0) {
                t.Respawn();
            }
            score += 100;
            return;
        }
    }
    
    // Colisión inmediata con bases enemigas
    if (baseA.GetX() == bulletX && baseA.GetY() == bulletY && tank.GetTeam() != 'A') {
        baseA.TakeDamage(1, tank.GetTeam());
        return;
    }
    if (baseB.GetX() == bulletX && baseB.GetY() == bulletY && tank.GetTeam() != 'B') {
        baseB.TakeDamage(1, tank.GetTeam());
        return;
    }
    
    // Sin colisión inmediata, crear la bala normalmente
    bullets.emplace_back(bulletX, bulletY, tank.GetDirection(), tank.GetTeam());
}

void GameState::RespawnTank(Tank& tank) {
    tank.Respawn();
}

//// SISTEMA DE BALAS ////

void GameState::UpdateBullets() {
    for (auto& bullet : bullets) {
        if (bullet.IsActive()) {
            bullet.Move();
        }
    }
}

void GameState::CheckBulletCollisions() {
    for (auto& bullet : bullets) {
        if (!bullet.IsActive()) continue;
        
        int bx = bullet.GetX();
        int by = bullet.GetY();
        
        // Fuera del mapa
        if (!IsValidPosition(bx, by)) {
            bullet.Deactivate();
            continue;
        }
        
        // Colisión con paredes
        for (auto& wall : walls) {
            if (wall->GetX() == bx && wall->GetY() == by) {
                wall->TakeDamage(1);
                bullet.Deactivate();
                break;
            }
        }
        if (!bullet.IsActive()) continue;
        
        // Colisión con tanques equipo A
        for (auto& tank : teamA_tanks) {
            if (tank.IsAlive() && tank.GetX() == bx && tank.GetY() == by && bullet.GetTeam() != 'A') {
                tank.TakeDamage(1);
                bullet.Deactivate();
                if (tank.GetHealth() == 0 && tank.GetLives() > 0) {
                    tank.Respawn();
                }
                score += 100;
                break;
            }
        }
        if (!bullet.IsActive()) continue;
        
        // Colisión con tanques equipo B
        for (auto& tank : teamB_tanks) {
            if (tank.IsAlive() && tank.GetX() == bx && tank.GetY() == by && bullet.GetTeam() != 'B') {
                tank.TakeDamage(1);
                bullet.Deactivate();
                if (tank.GetHealth() == 0 && tank.GetLives() > 0) {
                    tank.Respawn();
                }
                score += 100;
                break;
            }
        }
        if (!bullet.IsActive()) continue;
        
        // Colisión con bases
        if (baseA.GetX() == bx && baseA.GetY() == by && bullet.GetTeam() != 'A') {
            baseA.TakeDamage(1, bullet.GetTeam());
            bullet.Deactivate();
        }
        if (baseB.GetX() == bx && baseB.GetY() == by && bullet.GetTeam() != 'B') {
            baseB.TakeDamage(1, bullet.GetTeam());
            bullet.Deactivate();
        }
    }
    
    // Eliminar balas inactivas
    bullets.erase(
        remove_if(bullets.begin(), bullets.end(), 
            [](const Bullet& b) { return !b.IsActive(); }),
        bullets.end()
    );
}

void GameState::RemoveDestroyedWalls() {
    walls.erase(
        remove_if(walls.begin(), walls.end(),
            [](const shared_ptr<Wall>& w) { return w->IsDestroyed(); }),
        walls.end()
    );
}

void GameState::CheckGameOver() {
    // Base A destruida -> gana B
    if (!baseA.IsAlive()) {
        gameOver = true;
        winnerTeam = 'B';
        return;
    }
    
    // Base B destruida -> gana A
    if (!baseB.IsAlive()) {
        gameOver = true;
        winnerTeam = 'A';
        return;
    }
    
    // Todos los tanques de A muertos -> gana B
    bool anyAliveA = false;
    for (const auto& tank : teamA_tanks) {
        if (tank.IsAlive()) { anyAliveA = true; break; }
    }
    if (!anyAliveA && !teamA_tanks.empty()) {
        gameOver = true;
        winnerTeam = 'B';
        return;
    }
    
    // Todos los tanques de B muertos -> gana A
    bool anyAliveB = false;
    for (const auto& tank : teamB_tanks) {
        if (tank.IsAlive()) { anyAliveB = true; break; }
    }
    if (!anyAliveB && !teamB_tanks.empty()) {
        gameOver = true;
        winnerTeam = 'A';
        return;
    }
}

//// DETECCIÓN DE COLISIONES ////

bool GameState::IsValidPosition(int x, int y) const {
    return x >= 0 && x < boardSize && y >= 0 && y < boardSize;
}

bool GameState::IsBlocked(int x, int y) const {
    return IsBlockedByWall(x, y) || IsBlockedByTank(x, y);
}

bool GameState::IsBlockedByWall(int x, int y) const {
    for (const auto& wall : walls) {
        if (wall->GetX() == x && wall->GetY() == y) {
            return true;
        }
    }
    return false;
}

bool GameState::IsBlockedByTank(int x, int y, const Tank* ignoreTank) const {
    for (const auto& tank : teamA_tanks) {
        if (&tank != ignoreTank && tank.IsAlive() && tank.GetX() == x && tank.GetY() == y) {
            return true;
        }
    }
    for (const auto& tank : teamB_tanks) {
        if (&tank != ignoreTank && tank.IsAlive() && tank.GetX() == x && tank.GetY() == y) {
            return true;
        }
    }
    return false;
}

//// CONSULTAS DE ESTADO ////

Tank* GameState::GetTankById(int id) {
    for (auto& tank : teamA_tanks) {
        if (tank.GetId() == id) return &tank;
    }
    for (auto& tank : teamB_tanks) {
        if (tank.GetId() == id) return &tank;
    }
    return nullptr;
}

vector<Tank*> GameState::GetAliveTanks(char team) {
    vector<Tank*> alive;
    auto& tanks = (team == 'A') ? teamA_tanks : teamB_tanks;
    for (auto& tank : tanks) {
        if (tank.IsAlive()) {
            alive.push_back(&tank);
        }
    }
    return alive;
}

Base& GameState::GetBaseByTeam(char team) {
    return (team == 'A') ? baseA : baseB;
}

//// GETTERS ////

vector<Tank>& GameState::GetTeamATanks() { return teamA_tanks; }
vector<Tank>& GameState::GetTeamBTanks() { return teamB_tanks; }
vector<shared_ptr<Wall>>& GameState::GetWalls() { return walls; }
vector<Bullet>& GameState::GetBullets() { return bullets; }
const vector<Tank>& GameState::GetTeamATanks() const { return teamA_tanks; }
const vector<Tank>& GameState::GetTeamBTanks() const { return teamB_tanks; }
const vector<shared_ptr<Wall>>& GameState::GetWalls() const { return walls; }
const vector<Bullet>& GameState::GetBullets() const { return bullets; }
int GameState::GetBoardSize() const { return boardSize; }
int GameState::GetScore() const { return score; }
int GameState::GetActualFrame() const { return actualFrame; }
char GameState::GetWinnerTeam() const { return winnerTeam; }

const Base& GameState::GetBaseByTeam(char team) const {
    return (team == 'A') ? baseA : baseB;
}

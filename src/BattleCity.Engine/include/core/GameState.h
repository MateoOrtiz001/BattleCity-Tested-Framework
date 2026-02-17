#pragma once
#include <vector>
#include <string>
#include <memory>
#include "../entities/Tank.h"
#include "../entities/Wall.h"
#include "../entities/Base.h"
#include "../entities/Bullet.h"

using namespace std;

class GameState {
    public:
        // Inicialización y control
        void Initialize(const vector<string>& layout, int tickLimit = 500);
        void Update();
        void Reset();
        bool IsGameOver() const;

        // Acciones de tanques
        bool MoveTank(Tank& tank, int direction);
        void TankShoot(Tank& tank);
        void RespawnTank(Tank& tank);

        // Detección de colisiones
        bool IsValidPosition(int x, int y) const;
        bool IsBlocked(int x, int y) const;
        bool IsBlockedByWall(int x, int y) const;
        bool IsBlockedByTank(int x, int y, const Tank* ignoreTank = nullptr) const;

        // Consultas de estado
        Tank* GetTankById(int id);
        vector<Tank*> GetAliveTanks(char team);
        Base& GetBaseByTeam(char team);

        // Getters existentes
        vector<Tank>& GetTeamATanks();
        vector<Tank>& GetTeamBTanks();
        vector<shared_ptr<Wall>>& GetWalls();
        vector<Bullet>& GetBullets();
        const vector<Tank>& GetTeamATanks() const;
        const vector<Tank>& GetTeamBTanks() const;
        const vector<shared_ptr<Wall>>& GetWalls() const;
        const vector<Bullet>& GetBullets() const;
        int GetBoardSize() const;
        int GetScore() const;
        int GetActualFrame() const;
        char GetWinnerTeam() const;
        const Base& GetBaseByTeam(char team) const;

        // Cheats
        void SpawnTank(int x, int y, char team);
        void SpawnTanks(int count, char team);
        void RemoveTank(int tankId);
        void RemoveAllTanks(char team);

        void HealTank(int tankId, int amount);
        void HealAllTanks(char team, int amount);
        void SetTankLives(int tankId, int lives);
        void HealBase(char team, int amount);
        void SetBaseHealth(char team, int health);

        void KillTank(int tankId);
        void KillAllTanks(char team);
        void DestroyBase(char team);

        void ChangeWallType(int x, int y, const std::string& newType);
        void ChangeAllWallsType(const std::string& newType);
        void AddWall(int x, int y, const std::string& type);
        void RemoveWall(int x, int y);
        void ClearWalls();

        void ForceRestart();
        void ForceGameOver(char winner);
        void SetTickLimit(int newLimit);
        void SetScore(int newScore);
        void PauseGame();
        void ResumeGame();

        void ClearAllBullets();
        void SpawnBullet(int x, int y, int direction, char team);

        int GetTankCount(char team) const;
        int GetAliveCount(char team) const;
        int GetTickLimit() const;
        bool IsPaused() const;

    private:
        // Funciones internas
        void UpdateBullets();
        void CheckBulletCollisions();
        void CheckGameOver();
        void RemoveDestroyedWalls();

        // Estado del juego
        int boardSize = 0;
        vector<Tank> teamA_tanks;
        vector<Tank> teamB_tanks;
        vector<shared_ptr<Wall>> walls;
        vector<Bullet> bullets;
        bool gameOver = false;
        char winnerTeam = ' ';
        int score = 0;
        int tickLimit = 500;
        int actualFrame = 0;
        Base baseA;
        Base baseB;
        bool paused = false;
        
        // Layout original para reset
        vector<string> originalLayout;
};
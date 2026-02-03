#pragma once
#include <vector>
#include <string>
#include <memory> 

using namespace std;

class GameState{
    public:
        void Initialize(const vector<string>& layout);


    private:
        vector<class Tank> teamA_tanks;
        vector<class Tank> teamB_tanks;
        vector<shared_ptr<class Wall>> walls;
        int score;
        int timelimit = 500;
        class Base* base;
};
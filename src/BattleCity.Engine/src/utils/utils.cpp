#include "include/utils/Utils.h"
#include <cmath>


int ManhattanDistance(int x1, int y1, int x2, int y2) {
    return abs(x1 - x2) + abs(y1 - y2);
}

int EuclideanDistance(int x1, int y1, int x2, int y2) {
    return static_cast<int>(sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2)));
}

int Clamp(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}
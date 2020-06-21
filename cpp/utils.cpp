#include "utils.h"

int RoundLit(double x) {

    if (x >= 0) { return (x - (int)x >= .5) ? (int)x + 1 : (int)x; }
    else { return -((-x - (int)-x >= .5) ? (int)-x + 1 : (int)-x); }
}
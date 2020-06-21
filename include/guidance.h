#pragma once

#define _USE_MATH_DEFINES

#include <cstdlib>
#include <math.h>
#include "typedefs.h"
#include "ks.h"
#include "logger.h"

bool guidance_init();
void guidance_cleanup();
bool add_waypoint();
bool build();
double slope(Waypoint*, Waypoint*);
void step();
Waypoint* new_current_waypoint();
double sq(double);
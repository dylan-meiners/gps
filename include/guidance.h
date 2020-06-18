#pragma once

#include <cstdlib>
#include "typedefs.h"
#include "ks.h"
#include "logger.h"

bool guidance_init();
void guidance_cleanup();
bool add_waypoint();
bool build();
Waypoint* new_current_waypoint();
double sq(double);
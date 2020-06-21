#pragma once

#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#include <cstdlib>
#include "typedefs.h"
#include "ks.h"
#include "timing.h"
#include "logger.h"
#include "utils.h"

bool console_init();
bool update();
void clear_pages();
void clearLine(uint);
void printHL(uint);
void wstrcpy(wchar_t*, wchar_t*);
void plot(double, double, int);
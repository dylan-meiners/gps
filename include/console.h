#pragma once

#include <Windows.h>
#include <cstdlib>
#include "typedefs.h"
#include "ks.h"
#include "timing.h"
#include "logger.h"

bool console_init();
bool update();
void clear_pages();
void clearLine(uint);
void printHL(uint);
void wstrcpy(wchar_t*, wchar_t*);
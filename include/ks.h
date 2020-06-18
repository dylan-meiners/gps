#pragma once

#include <Windows.h>
#include "typedefs.h"

#define USE_FAKE_DATA

const uint WIDTH = 200;
const uint HEIGHT = 45;
const uint TOTAL_CHARS = WIDTH * HEIGHT;
const uint NUM_INITIAL_WAYPOINT = 10;
const uint CONTENT_START = WIDTH * 12;
const uint CONTENT_STOP = WIDTH * (HEIGHT - 3);

const wchar_t LIGHT = 0x2591;
const wchar_t MEDIUM = 0x2592;
const wchar_t DARK = 0x2593;
const wchar_t FULL = 0x2588;

const double LAT_DEG_FEET = 365341.0; //45° to 50°
const double LONG_DEG_FEET = 258398.0; //40° to 45°
const double LAT_MIN_TO_FEET_MULTIPLIER = LAT_DEG_FEET / 60.0;
const double LONG_MIN_TO_FEET_MULTIPLIER = LONG_DEG_FEET / 60.0;

const uint DT_MAX_LEN = 2;
const uint ACTIVE_MAX_LEN = 7;
const uint DEGREES_MAX_LEN = 3;
const uint CHAR_MAX_LEN = 1;
const uint GENERIC_DOUBLE_MAX_LEN = 8;
const uint SINGLE_SPACE = 1;
const uint SPACES = 10;
const uint GPRMC_START_OFFSET = WIDTH * 5;

const uint FLAG_END = 0x01;
const uint FLAG_SERIAL_END = 0x02;
const uint FLAG_DO_DEBUG = 0x04;
const uint FLAG_CONSOLE_END = 0x08;
#pragma once

#include "typedefs.h"
#include <SerialPort/SerialPort.h>
#include "typedefs.h"
#include "ks.h"
#include "timing.h"
#include "logger.h"

void serial_main();
bool valid(uchar*, const bool use_chksm);
uint uchar_star_to_uint(uchar*, uint);
double parse_double_to_comma(uchar*);
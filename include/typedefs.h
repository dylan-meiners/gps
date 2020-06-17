#pragma once

typedef unsigned int uint;
typedef unsigned char uchar;

struct Position {
	int deg;
	double min;
	uchar dir;
};

struct Waypoint {
	Position* latitude;
	Position* longitude;
	double rel_x;
	double rel_y;
};

struct GPRMC_STRUCT {
	int t_h;
	int t_m;
	int t_s;
	int active;
	Position latitude;
	Position longitude;
	double v;
	double ta;
	int d_d;
	int d_m;
	int d_y;
	double mv;
	uchar mv_dir;
	uchar mode;
};
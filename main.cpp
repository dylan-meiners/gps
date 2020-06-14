#include <stdint.h>
#include <iostream>
#include <conio.h>
#include <SerialPort/SerialPort.h>
#include <time.h>
#include <ctime>
#include <locale.h> //° symbol
#include <thread>

/*
IMPORTANT - This code only for small distance applications.  It "converts"
			longitude and latitude to flat "coordinates".  The conversion units are
			LAT_DEG_FEET and LONG_DEG_FEET, which means the number of feet in one degree
			of latitude or longitude, respectively.
*/

//#define USE_FAKE_DATA

typedef unsigned int uint;
typedef unsigned char uchar;

const uint NUM_INITIAL_WAYPOINT = 10;

const int LAT_DEG_FEET = 364000;
const int LONG_DEG_FEET = 288200;

enum Direction {
	NORTH,
	SOUTH,
	EAST,
	WEST
};

typedef struct Position {
	int deg;
	double min;
	uchar dir;
};

typedef struct Waypoint {
	Position* latitude;
	Position* longitude;
};

struct {
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
} GPRMC;

const uint FLAG_END = 0x01;
const uint FLAG_SERIAL_END = 0x02;
uint FLAGS = 0;

uchar buffer[1024] = { 0 };

void serial_main();
Waypoint* new_current_waypoint();
bool valid(uchar*, const bool);
uint uchar_star_to_uint(uchar*, uint);
double parse_double_to_comma(uchar*);
void print(std::string);
void print(int);
void print(double);
void print(float);
void print(uint);
void printGPRMC();

int main() {

	setlocale(LC_ALL, "");

	Waypoint** waypoints = (Waypoint**)malloc(NUM_INITIAL_WAYPOINT * sizeof(Waypoint*)); //array of Waypoint pointers
	for (uint i = 0; i < NUM_INITIAL_WAYPOINT; i++) waypoints[i] = NULL;
	if (waypoints == NULL) {

		fprintf(stderr, "FATAL: ERROR | %s | MALLOC FAILED CREATING waypoints!\n", __FUNCTION__);
		return false;
	}
	Waypoint** next_open = waypoints; //pointer to next open slot
	Waypoint** last_waypoint_slot = waypoints + NUM_INITIAL_WAYPOINT - 1; //pointer to last slot in array

	std::thread serial_thread(serial_main);

	bool running = true;
	while (running) {

		if (_kbhit()) {

			char c = _getch();
			switch ((int)c) {

				case 27: //ESC
					running = false;
					break;

				case 32: { //SPACE
					
					if (next_open == last_waypoint_slot) {

						uint num_new_slots = last_waypoint_slot - waypoints + NUM_INITIAL_WAYPOINT;
						Waypoint** new_waypoints = (Waypoint**)malloc(num_new_slots * sizeof(Waypoint*));
						for (uint i = 0; i < num_new_slots; i++) new_waypoints[i] = NULL;
						if (new_waypoints == NULL) {

							fprintf(stderr, "FATAL: ERROR | %s | MALLOC FAILED CREATING new_waypoints!\n", __FUNCTION__);
							return false;
						}
						for (uint i = 0; i < num_new_slots - NUM_INITIAL_WAYPOINT - 1; i++) {

							new_waypoints[i] = waypoints[i];
						}
						free(waypoints);
						waypoints = new_waypoints;
						last_waypoint_slot = waypoints + num_new_slots;
						next_open = waypoints + num_new_slots - NUM_INITIAL_WAYPOINT - 1;
					}
					Waypoint* w = new_current_waypoint();
					if (w == nullptr) fprintf(stderr, "ERROR | %s | COULD NOT CREATE A NEW WAYPOINT\n", __FUNCTION__);
					else printf(
						"Successfully created a new waypoint at curent location: %d %f %c    %d %f %c\n",
						w->latitude->deg, w->latitude->min, w->latitude->dir,
						w->longitude->deg, w->longitude->min, w->longitude->dir
					);
					*next_open = w;
					next_open++;
					break;
				}

				case 13:
					for (uint i = 0; i < last_waypoint_slot - waypoints; i++) {

						if (waypoints[i] != NULL) {

							printf(
								"Waypoint %d: %d %f %c    %d %f %c\n",
								i + 1,
								waypoints[i]->latitude->deg, waypoints[i]->latitude->min, waypoints[i]->latitude->dir,
								waypoints[i]->longitude->deg, waypoints[i]->longitude->min, waypoints[i]->longitude->dir
							);
						}
						else printf("Waypoint: %d: NULL\n", i + 1);
					}
					break;

				case 99: //'c'
					system("CLS");
					break;
				
				default:
					break;
			}
		}

		if (FLAGS & FLAG_SERIAL_END) {

			running = false;
			printf("SERIAL THREAD WANTS TO END PREMATURELY, EXITING!\n");
		}
	}

	FLAGS ^= FLAG_END;

	printf("\n\nCleaning up...\n");

	free(waypoints);

	printf("Waiting for serial thread to rejoin...\n");
	serial_thread.join();
	printf("Serial thread has rejoined, exiting...\n");

	return 0;
}

void serial_main() {

	const char* PORT = "COM8";
	SerialPort ardi(L"COM8", true, false);
	if (ardi.Setup()) printf("Successfully set up serial port on %s!\n", PORT);
	else {

		fprintf(stderr, "FATAL ERROR | COULD NOT SETUP ARDUINO: %d\n", GetLastError());
		FLAGS ^= FLAG_SERIAL_END;
	}
	uint bytesRead = 0;

#ifdef USE_FAKE_DATA
	//======================================TESTING======================================================
	const char* src = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\n\x04";
	//const char* src = "$GPRMC,,,,,,,,00000000.123456789,,,*00\n\x04";
	uint len = strlen(src);
	uchar* test = (uchar*)malloc(sizeof(uchar) * len); //one less because it is a null terminated string
	if (test == NULL) {

		fprintf(stderr, "ERROR | %s | first byte must be a $\n", __FUNCTION__);
	}
	else {

		memcpy(test, src, len);
	}
	//===================================================================================================
#endif

	bool searching = true;
	while ((FLAGS & FLAG_END) == 0 && (FLAGS & FLAG_SERIAL_END) == 0) {

#ifndef USE_FAKE_DATA
		bytesRead = ardi.ReadAll(buffer);
		buffer[bytesRead] = 0x04; //EOT
		//printf("%s\n", buffer);
		uchar* ptr = buffer;
#else
		uchar* ptr = test;
#endif
		while (searching && (*ptr != 0x04)) {

			while (*ptr != 0x04 && *++ptr != 'G');
			if (*ptr == 0x04) break;
			else searching = !(/**++ptr == 'G' && */*++ptr == 'P' && *++ptr == 'R' && *++ptr == 'M' && *++ptr == 'C');
		}
		if (searching) fprintf(stderr, "ERROR | %s | COULD NOT FIND GPRMC IN BUFFER\n\n", __FUNCTION__);
		else {

			ptr -= 5;
			if (valid(ptr, true)) {

				ptr += 7; //go from $ to first digit after type ident. eg. $GPRMC, --> 1 <-- 2
				if (*ptr == ',') {

					GPRMC.t_h = -1;
					GPRMC.t_m = -1;
					GPRMC.t_s = -1;

					ptr++; //skip, no data available
				}
				else {

					GPRMC.t_h = uchar_star_to_uint(ptr, 2);
					ptr += 2;
					GPRMC.t_m = uchar_star_to_uint(ptr, 2);
					ptr += 2;
					GPRMC.t_s = uchar_star_to_uint(ptr, 2);

					while (*++ptr != ','); //to active or void
					ptr++;
				}

				if (*ptr == ',') {

					GPRMC.active = -1;

					ptr++; //skip, no data available
				}
				else {

					if (*ptr == 'A') GPRMC.active = 1;
					else if (*ptr == 'V') GPRMC.active = 0;
					else GPRMC.active = -1;

					ptr += 2; //to lat
				}

				uint acc = 0;
				if (*ptr == ',') {

					GPRMC.latitude.deg = -1;
					GPRMC.latitude.min = -1.0;

					ptr++; //skip, no data available
				}
				else {

					while (*++ptr != '.'); //go to decimal in lat or long
					ptr -= 2; //go to start of min
					while (*--ptr != ',') { acc++; } //ptr is at ',' when done
					ptr++; //go to start of degress
					GPRMC.latitude.deg = uchar_star_to_uint(ptr, acc);
					ptr += acc; //go to start of min
					GPRMC.latitude.min = parse_double_to_comma(ptr);

					while (*++ptr != ','); //to lat dir
					ptr++;
				}
				if (*ptr == ',') {

					GPRMC.latitude.dir = 63;

					ptr++; //skip, no data available
				}
				else {

					GPRMC.latitude.dir = *ptr;

					ptr += 2; //to longitude
				}

				acc = 0;

				if (*ptr == ',') {

					GPRMC.longitude.deg = -1;
					GPRMC.longitude.min = -1.0;

					ptr++; //skip, no data available
				}
				else {

					while (*++ptr != '.'); //go to decimal in lat or long
					ptr -= 2; //go to start of min
					while (*--ptr != ',') { acc++; } //ptr is at ',' when done
					ptr++; //go to start of degress
					GPRMC.longitude.deg = uchar_star_to_uint(ptr, acc);
					ptr += acc; //go to start of min
					GPRMC.longitude.min = parse_double_to_comma(ptr);

					while (*++ptr != ','); //to long dir
					ptr++;
				}
				if (*ptr == ',') {

					GPRMC.longitude.dir = 63;

					ptr++; //skip, no data available
				}
				else {

					GPRMC.longitude.dir = *ptr;

					ptr += 2; //to velocity
				}

				if (*ptr == ',') {

					GPRMC.v = -1.0;

					ptr++; //skip, no data available
				}
				else {

					GPRMC.v = parse_double_to_comma(ptr);

					while (*++ptr != ','); //to track angle
					ptr++;
				}

				if (*ptr == ',') {

					GPRMC.ta = -1.0;

					ptr++;//skip, no data available
				}
				else {

					GPRMC.ta = parse_double_to_comma(ptr);

					while (*++ptr != ','); //to date
					ptr++;
				}

				if (*ptr == ',') {

					GPRMC.d_d = -1;
					GPRMC.d_m = -1;
					GPRMC.d_y = -1;

					ptr++; //skip, no data available
				}
				else {

					GPRMC.d_d = uchar_star_to_uint(ptr, 2);
					ptr += 2;
					GPRMC.d_m = uchar_star_to_uint(ptr, 2);
					ptr += 2;
					GPRMC.d_y = uchar_star_to_uint(ptr, 2);

					ptr += 3; //to magnetic variation
				}

				if (*ptr == ',') {

					GPRMC.mv = -1.0;

					ptr++; //skip, no data available
				}
				else {

					GPRMC.mv = parse_double_to_comma(ptr);

					while (*++ptr != ','); //to mv dir
					ptr++;
				}

				if (*ptr == ',') {

					GPRMC.mv_dir = 63;

					ptr++; //skip, no data available
				}
				else {

					GPRMC.mv_dir = *ptr;

					ptr++; //to mode
				}

				if (*ptr != '*') GPRMC.mode = 63;
				else GPRMC.mode = *ptr;

				//if ((FLAGS & FLAG_END) == 0 && (FLAGS & FLAG_SERIAL_END) == 0) printGPRMC();
			}
		}

		memset(buffer, NULL, 1024);
		searching = true;
	}

#ifdef USE_FAKE_DATA
	free(test);
#endif
}

Waypoint* new_current_waypoint() {

	Waypoint* w = (Waypoint*)malloc(sizeof(Waypoint));
	if (w == NULL) {

		fprintf(stderr, "ERROR | %s | MALLOC FAILED WHEN CREATING WAYPOINT STRUCT\n", __FUNCTION__);
		return nullptr;
	}

	Position* latitude = (Position*)malloc(sizeof(Position));
	if (latitude == NULL) {

		fprintf(stderr, "ERROR | %s | MALLOC FAILED WHEN CREATING LATITUDE STRUCT\n", __FUNCTION__);
		return nullptr;
	}
	else {

		latitude->deg = GPRMC.latitude.deg;
		latitude->min = GPRMC.latitude.min;
		latitude->dir = GPRMC.latitude.dir;
	}

	Position* longitude = (Position*)malloc(sizeof(Position));
	if (longitude == NULL) {

		fprintf(stderr, "ERROR | %s | MALLOC FAILED WHEN CREATING LONGITUDE STRUCT\n", __FUNCTION__);
		return nullptr;
	}
	else {

		longitude->deg = GPRMC.longitude.deg;
		longitude->min = GPRMC.longitude.min;
		longitude->dir = GPRMC.longitude.dir;
	}

	w->latitude = latitude;
	w->longitude = longitude;
	return w;
}

bool valid(uchar* buffer, const bool use_chksm = true) {
	//TODO: Check if more than one *
	uchar* ptr = buffer;
	if (*ptr != '$') {

		fprintf(stderr, "ERROR | %s | first byte must be a $\n", __FUNCTION__);
		return false; //first byte must be a $
	}
	/*ptr += len - 1; //go to the last byte
	if (*ptr != '\n') {

		fprintf(stderr, "ERROR | %s | last byte must be a newline, tested was a %d\n", __FUNCTION__, (int)*ptr);
		return false; //last byte must be a newline
	}
	ptr -= len - 1; //go back to $*/
	bool searching = true;
	while (searching && *++ptr != '\n') { searching = *ptr != '*'; } //make sure to not increment if we are done searching
	if (searching) {
		
		fprintf(stderr, "ERROR | %s | could not find *\n", __FUNCTION__);
		return false; //could not find *
	}
	if (!use_chksm) return true;

	//met the above conditions, calculate the checksum
	ptr++; //ptr is currently pointing to *, so get it to the next char
	uint checksum = 0;
	uint dec_form[2] = { 0 };
	for (uint i = 0; i < 2; i++) {

		if (*ptr >= '0' && *ptr <= '9') dec_form[i] = *ptr - 48; //if a digit, convert form ascii to int
		else if (*ptr >= 'a' && *ptr <= 'f') dec_form[i] = *ptr - 97 + 10;
		else if (*ptr >= 'A' && *ptr <= 'F') dec_form[i] = *ptr - 65 + 10;
		
		checksum += dec_form[i] * (uint)pow(16, 1 - i);
		ptr++;
	}
	while (*--ptr != '$'); //go to $
	uint8_t acc = 0;
	while (*++ptr != '*') { acc ^= *ptr; }
	if (acc != checksum) {

		fprintf(stderr, "ERROR | %s | the checksum is not valid; expected %d, got %d\n", __FUNCTION__, checksum, acc);
		return false;
	}
	return true;
}

uint uchar_star_to_uint(uchar* buffer, uint digits) {

	uchar* ptr = buffer;
	uint acc = 0;
	for (uint i = 0; i < digits; i++) {
		
		acc += (*ptr - 48) * (uint)pow(10, digits - 1 - i);
		ptr++;
	}
	return acc;
}

double parse_double_to_comma(uchar* start) {

	uchar* ptr = start;
	double res = 0.0;
	uint acc = 1;
	while (*++ptr != '.') { acc++; }
	ptr -= acc;
	res = uchar_star_to_uint(ptr, acc);
	ptr += acc;
	acc = 0;
	while (*++ptr != ',') { acc++; }
	ptr -= acc;
	double temp = uchar_star_to_uint(ptr, acc);
	while (temp > 1) {

		temp /= 10.0;
		acc--;
	}
	if (acc > 0) { for (uint i = 0; i < acc; i++) { temp /= 10.0; } }
	return res + temp;
}

void print(std::string s) { std::cout << s << std::endl; }
void print(double d) { std::cout << d << std::endl; }
void print(int i) { std::cout << i << std::endl; }
void print(float f) { std::cout << f << std::endl; }
void print(uint u) { std::cout << u << std::endl; }

void printGPRMC() {

	time_t t = time(NULL);
	struct tm time_info;
	char t_buffer[32];
	localtime_s(&time_info, &t);
	std::strftime(t_buffer, 32, "%m/%e/%Y-%H:%M:%S", &time_info);

	printf(
		"%s > %d %d %d    %s    %d° %f' %c    %d° %f' %c    %f    %f    %d %d %d    %f %c    %c\n",
		t_buffer,
		GPRMC.t_h,
		GPRMC.t_m,
		GPRMC.t_s,
		(GPRMC.active > -1) ? (GPRMC.active ? "active" : "void") : "unknown",
		GPRMC.latitude.deg,
		GPRMC.latitude.min,
		GPRMC.latitude.dir,
		GPRMC.longitude.deg,
		GPRMC.longitude.min,
		GPRMC.longitude.dir,
		GPRMC.v,
		GPRMC.ta,
		GPRMC.d_d,
		GPRMC.d_m,
		GPRMC.d_y,
		GPRMC.mv,
		GPRMC.mv_dir,
		GPRMC.mode
	);
}
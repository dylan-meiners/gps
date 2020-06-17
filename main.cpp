#include <stdint.h>
#include <iostream>
#include <stdio.h>
#include <conio.h>
#include <SerialPort/SerialPort.h>
#include <chrono>
#include <time.h>
#include <ctime>
#include <locale.h> //° symbol
#include <thread>
#include <Windows.h>
#include <sstream>
#include <string>
#include <iomanip>

/*
IMPORTANT - This code only for small distance applications.  It "converts"
			longitude and latitude to flat "coordinates".  The conversion units are
			LAT_DEG_FEET and LONG_DEG_FEET, which means the number of feet in one degree
			of latitude or longitude, respectively.
*/

#define USE_FAKE_DATA

typedef unsigned int uint;
typedef unsigned char uchar;

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

/*
Because of where I live, the coordinate plane will be like this:
								+Y

								|
								|
								|
								|
								|
								|
+X  ----------------------------|----------------------------  -X
								|
								|
								|
								|
								|
								|
								|

								-Y
*/

const uint DT_MAX_LEN = 2;
const uint ACTIVE_MAX_LEN = 7;
const uint DEGREES_MAX_LEN = 3;
const uint CHAR_MAX_LEN = 1;
const uint GENERIC_DOUBLE_MAX_LEN = 8;
const uint SINGLE_SPACE = 1;
const uint SPACES = 10;
const uint GPRMC_START_OFFSET = WIDTH * 5;

wchar_t buf[256];

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
	double rel_x;
	double rel_y;
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
const uint FLAG_DO_DEBUG = 0x04;
const uint FLAG_CONSOLE_END = 0x08;
uint FLAGS = 0;

uchar buffer[1024] = { 0 };

void serial_main();
Waypoint* new_current_waypoint();
bool valid(uchar*, const bool);
uint uchar_star_to_uint(uchar*, uint);
double parse_double_to_comma(uchar*);
double sq(double x) { return x * x; }
void printGPRMC();
void printWaypoints();
void clear_pages();
void clearLine(uint);
void printHL(uint);
void wstrcpy(wchar_t*, wchar_t*);
std::string current_time();

wchar_t** screens = (wchar_t**)malloc(sizeof(wchar_t*)); //only one to start
wchar_t* master = (wchar_t*)malloc(TOTAL_CHARS * sizeof(wchar_t)); //master buffer
wchar_t* page_one = (wchar_t*)malloc(TOTAL_CHARS * sizeof(wchar_t)); //page one
HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, NULL, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
DWORD dwBytesWritten = 0;

Waypoint** waypoints = (Waypoint**)malloc(NUM_INITIAL_WAYPOINT * sizeof(Waypoint*)); //array of Waypoint pointers
Waypoint** next_open = waypoints; //pointer to next open slot
Waypoint** last_waypoint_slot = waypoints + NUM_INITIAL_WAYPOINT - 1; //pointer to last slot in array

double* distances = nullptr;
double* last_distance_slot = nullptr;

FILE* fp;

uint content_page_index = 0;
uint num_content_pages = 1;
int main() {

	fopen_s(&fp, "o.txt", "w");
	
	if (master == NULL) {

		fprintf(fp, "[%s] FATAL: ERROR | At function: \"%s\", line: %d | MALLOC FAILED CREATING master!\n", current_time().c_str(), __FUNCTION__, __LINE__);
		return false;
	}
	if (page_one == NULL) {

		fprintf(fp, "[%s] FATAL: ERROR | At function: \"%s\", line: %d | MALLOC FAILED CREATING page_one!\n", current_time().c_str(), __FUNCTION__, __LINE__);
		return false;
	}
	if (screens == NULL) {

		fprintf(fp, "[%s] FATAL: ERROR | At function: \"%s\", line: %d | MALLOC FAILED CREATING screens (plural)!\n", current_time().c_str(), __FUNCTION__, __LINE__);
		return false;
	}
	screens[0] = page_one;

	SetConsoleActiveScreenBuffer(hConsole);
	setlocale(LC_ALL, "");

	if (waypoints == NULL) {

		fprintf(fp, "[%s] FATAL: ERROR | At function: \"%s\", line: %d | MALLOC FAILED CREATING waypoints!\n", current_time().c_str(), __FUNCTION__, __LINE__);
		return false;
	}
	for (uint i = 0; i < NUM_INITIAL_WAYPOINT; i++) waypoints[i] = NULL;

	Position lat_init;
	Position long_init;
	lat_init.deg = -1;
	lat_init.min = -1.0;
	lat_init.dir = 63;
	long_init.deg = -1;
	long_init.min = -1.0;
	long_init.dir = 63;
	GPRMC.t_h = -1;
	GPRMC.t_m = -1;
	GPRMC.t_s = -1;
	GPRMC.active = 63;
	GPRMC.latitude = long_init;
	GPRMC.longitude = long_init;
	GPRMC.v = -1.0;
	GPRMC.ta = -1.0;
	GPRMC.d_d = -1;
	GPRMC.d_m = -1;
	GPRMC.d_y = -1;
	GPRMC.mv = -1.0;
	GPRMC.mv_dir = 63;
	GPRMC.mode = 63;

	std::thread serial_thread(serial_main);

	bool running = true;
	while (running) {

		if (_kbhit()) {

			/*
			KEY LAYOUT
			===================================================================
			q			Stop the program
			SPACE		Make a new waypoint off of current location and add it to waypoints
			ENTER		Print out waypoints
			t			Toggle debug
			b			Build route; connect waypoints
			left arrow	Switch to the previous page, if we can
			right arrow	Switch to the next page, if we can
			*/

			char c = _getch();
			switch (c) {

				case 'q': //ESC
					running = false;
					break;

				case 32: { //SPACE
#ifndef USE_FAKE_DATA
					fprintf(fp, "[%s] Attempting to create a new waypoint...\n", current_time().c_str());
					if (GPRMC.latitude.deg != -1 && GPRMC.latitude.min != -1 && GPRMC.latitude.dir != 63 &&
						GPRMC.longitude.deg != -1 && GPRMC.longitude.min != -1 && GPRMC.longitude.dir != 63) {

						if (next_open == last_waypoint_slot) {

							uint num_new_slots = last_waypoint_slot - waypoints + NUM_INITIAL_WAYPOINT + 1;
							Waypoint** new_waypoints = (Waypoint**)malloc(num_new_slots * sizeof(Waypoint*));
							if (new_waypoints == NULL) {

								fprintf(fp, "[%s] FATAL: ERROR | At function: \"%s\", line: %d | MALLOC FAILED CREATING new_waypoints!\n", current_time().c_str(), __FUNCTION__, __LINE__);
								return false;
							}
							for (uint i = 0; i < num_new_slots; i++) new_waypoints[i] = NULL;
							for (uint i = 0; i < num_new_slots - NUM_INITIAL_WAYPOINT - 1; i++) new_waypoints[i] = waypoints[i];
							free(waypoints);
							waypoints = new_waypoints;
							last_waypoint_slot = waypoints + num_new_slots - 1;
							next_open = waypoints + num_new_slots - NUM_INITIAL_WAYPOINT - 1;
						}
						if (next_open == nullptr) fprintf(fp, "[%s] Route already built, cannot add current position.\n", current_time().c_str());
						else {
						
							Waypoint* w = new_current_waypoint();
							if (w == nullptr) fprintf(fp, "[%s] ERROR | At function: \"%s\", line: %d | COULD NOT CREATE A NEW WAYPOINT\n", current_time().c_str(), __FUNCTION__, __LINE__);
							else fprintf(fp, 
								"[%s] Successfully created a new waypoint at curent location: %d %f %c    %d %f %c\n",
								current_time().c_str(),
								w->latitude->deg, w->latitude->min, w->latitude->dir,
								w->longitude->deg, w->longitude->min, w->longitude->dir
							);
							*next_open = w;
							next_open++;
						}
					}
					else fprintf(fp, "[%s] ERROR | At function: \"%s\", line: %d | CANNOT CREATE NEW WAYPOINT BECAUSE THERE IS NO CURRENT DATA!\n", current_time().c_str(), __FUNCTION__, __LINE__);
					fprintf(fp, "[%s] Finished attempting to create a new waypoint.\n", current_time().c_str());
#else
					fprintf(fp, "[%s] Attempting to create new waypoints from fake data...\n", current_time().c_str());
					Position* la1 = (Position*)malloc(sizeof(Position));
					Position* lo1 = (Position*)malloc(sizeof(Position));
					Position* la2 = (Position*)malloc(sizeof(Position));
					Position* lo2 = (Position*)malloc(sizeof(Position));
					Position* la3 = (Position*)malloc(sizeof(Position));
					Position* lo3 = (Position*)malloc(sizeof(Position));
					Position* la4 = (Position*)malloc(sizeof(Position));
					Position* lo4 = (Position*)malloc(sizeof(Position));
					if (la1 == NULL || lo1 == NULL || la2 == NULL || lo2 == NULL || la3 == NULL || lo3 == NULL || la4 == NULL || lo4 == NULL) {

						fprintf(fp, "[%s] FATAL: ERROR | At function: \"%s\", line: %d | MALLOC FAILED CREATING a fake Position!\n", current_time().c_str(), __FUNCTION__, __LINE__);
						return false;
					}

					la1->deg = 46;
					la1->min = 0.197166667;
					la1->dir = 'N';
					lo1->deg = 91;
					lo1->min = 18.225333333;
					lo1->dir = 'W';

					la2->deg = 46;
					la2->min = 1.587;
					la2->dir = 'N';
					lo2->deg = 91;
					lo2->min = 18.281166667;
					lo2->dir = 'W';

					la3->deg = 46;
					la3->min = 0.826166667;
					la3->dir = 'N';
					lo3->deg = 91;
					lo3->min = 19.053;
					lo3->dir = 'W';

					la4->deg = 46;
					la4->min = 0.105833333;
					la4->dir = 'N';
					lo4->deg = 91;
					lo4->min = 18.854333333;
					lo4->dir = 'W';

					Waypoint* w1 = (Waypoint*)malloc(sizeof(Waypoint));
					Waypoint* w2 = (Waypoint*)malloc(sizeof(Waypoint));
					Waypoint* w3 = (Waypoint*)malloc(sizeof(Waypoint));
					Waypoint* w4 = (Waypoint*)malloc(sizeof(Waypoint));
					if (w1 == NULL || w2 == NULL || w3 == NULL || w4 == NULL) {

						fprintf(fp, "[%s] FATAL: ERROR | At function: \"%s\", line: %d | MALLOC FAILED CREATING a fake Position!\n", current_time().c_str(), __FUNCTION__, __LINE__);
						return false;
					}

					w1->latitude = la1;
					w1->longitude = lo1;
					w1->rel_x = 0;
					w1->rel_y = 0;

					w2->latitude = la2;
					w2->longitude = lo2;
					w2->rel_x = 0;
					w2->rel_y = 0;

					w3->latitude = la3;
					w3->longitude = lo3;
					w3->rel_x = 0;
					w3->rel_y = 0;

					w4->latitude = la4;
					w4->longitude = lo4;
					w4->rel_x = 0;
					w4->rel_y = 0;

					waypoints[0] = w1;
					waypoints[1] = w2;
					waypoints[2] = w3;
					waypoints[3] = w4;
					fprintf(fp, "[%s] Finished attempting to create new waypoints from fake data.\n", current_time().c_str());
#endif
					printWaypoints();
					break;
				}

				case 116: //'t'
					FLAGS ^= FLAG_DO_DEBUG;
					if (FLAGS & FLAG_DO_DEBUG) fprintf(fp, "[%s] Enabled debug mode\n", current_time().c_str());
					else fprintf(fp, "[%s] Disabled debug mode\n", current_time().c_str());
					break;

				case 98: { //'b'

					fprintf(fp, "[%s] Building route...\n", current_time().c_str());
					//first, shave nulls
					uint acc = 0; //find number of open slots
					for (uint i = 0; i < last_waypoint_slot - waypoints; i++) if (waypoints[i] == NULL) acc++;

					uint num_new_slots = last_waypoint_slot - waypoints - acc; //shave off NULLs
					Waypoint** new_waypoints = (Waypoint**)malloc(num_new_slots * sizeof(Waypoint*));
					if (new_waypoints == NULL) {

						fprintf(fp, "[%s] FATAL: ERROR | At function: \"%s\", line: %d | MALLOC FAILED CREATING new_waypoints!\n", current_time().c_str(), __FUNCTION__, __LINE__);
						return false;
					}
					for (uint i = 0; i < num_new_slots; i++) new_waypoints[i] = waypoints[i];
					free(waypoints);
					waypoints = new_waypoints;
					last_waypoint_slot = waypoints + num_new_slots;
					next_open = nullptr;
					fprintf(fp, "[%s] Shaved off NULLs.\n", current_time().c_str());
					fprintf(fp, "[%s] You can no longer add waypoints, unless they are fake, in which case they will reset.\n", current_time().c_str());

					fprintf(fp, "[%s] Calculating distances...\n", current_time().c_str());
					double origin_x = (double)waypoints[0]->longitude->deg * LONG_DEG_FEET + waypoints[0]->longitude->min * LONG_MIN_TO_FEET_MULTIPLIER;
					double origin_y = (double)waypoints[0]->latitude->deg * LAT_DEG_FEET + waypoints[0]->latitude->min * LAT_MIN_TO_FEET_MULTIPLIER;
					for (uint i = 1; i < num_new_slots; i++) { //find relative distances for the waypoints, based on the first one being (0, 0)

						waypoints[i]->rel_x = (double)waypoints[i]->longitude->deg * LONG_DEG_FEET + waypoints[i]->longitude->min * LONG_MIN_TO_FEET_MULTIPLIER - origin_x - waypoints[i - 1]->rel_x;
						waypoints[i]->rel_y = (double)waypoints[i]->latitude->deg * LAT_DEG_FEET + waypoints[i]->latitude->min * LAT_MIN_TO_FEET_MULTIPLIER - origin_y - waypoints[i - 1]->rel_y;
					}

					distances = (double*)malloc(num_new_slots * sizeof(double)); //now, create an array of distances in between the waypoints
					for (uint i = 0; i < num_new_slots; i++) distances[i] = 0.0;
					if (distances == NULL) {

						fprintf(fp, "[%s] FATAL: ERROR | At function: \"%s\", line: %d | MALLOC FAILED CREATING distances!\n", current_time().c_str(), __FUNCTION__, __LINE__);
						return false;
					}
					for (uint i = 0; i < num_new_slots; i++) {

						distances[i] = sqrt(
							
							sq(waypoints[(i + 1 != num_new_slots) ? i + 1 : 0]->rel_x - waypoints[i]->rel_x) +
							sq(waypoints[(i + 1 != num_new_slots) ? i + 1 : 0]->rel_y - waypoints[i]->rel_y)
						);
					}
					last_distance_slot = distances + num_new_slots - 1;
					fprintf(fp, "[%s] Calculated distances.\n", current_time().c_str());
					fprintf(fp, "[%s] Route built! Waypoints are:\n", current_time().c_str());
					printWaypoints();
					double total_dist = 0.0;
					for (uint i = 0; i < num_new_slots; i++) total_dist += distances[i];
					fprintf(fp, "[%s] Total distance is %f miles\n", current_time().c_str(), total_dist / 5280);
					break;
				}

				case -32: {//start of escape sequence for _getch()

					switch (_getch()) {

						case 75: //left
							if (content_page_index > 0 && num_content_pages > 1) content_page_index--;
							break;

						case 77: //right
							if (content_page_index + 1 < num_content_pages) content_page_index++;
							break;

						default:
							break;
					}
				}
				
				default:
					char ch = c;
					break;
			}
		}

		if (FLAGS & FLAG_SERIAL_END) {

			running = false;
			fprintf(fp, "[%s] SERIAL THREAD WANTS TO END PREMATURELY, EXITING!\n", current_time().c_str());
		}

		//console stuff
		clear_pages();
		//len 22
		swprintf_s(buf, 256, L"                                                                                         Auto GPS Software V0.1                                                                                         \0");
		wstrcpy(master + WIDTH, buf);
		printHL(3);
		printHL(HEIGHT - 2);
		swprintf_s(
			buf,
			256,
			L"%s                                                                                            Page %d                                                                                            %s\0",
			(content_page_index != 0 && num_content_pages != 1) ? L"<----" : L"     ",
			content_page_index + 1,
			(content_page_index != num_content_pages && num_content_pages != 1) ? L"---->" : L"     "
		);
		wstrcpy(master + WIDTH * (HEIGHT - 1), buf);

		uint offset = GPRMC_START_OFFSET;
		swprintf_s(buf, 256, L"Current GPRMC Data:\0"); 
		wstrcpy(master + offset, buf);
		offset += SPACES + 19;
		swprintf_s(buf, 256, L"%d\0", GPRMC.t_h);
		wstrcpy(master + offset, buf);
		offset += DT_MAX_LEN + SINGLE_SPACE;
		swprintf_s(buf, 256, L"%d\0", GPRMC.t_m);
		wstrcpy(master + offset, buf);
		offset += DT_MAX_LEN + SINGLE_SPACE;;
		swprintf_s(buf, 256, L"%d\0", GPRMC.t_s);
		wstrcpy(master + offset, buf);
		offset += DT_MAX_LEN + SPACES;
		swprintf_s(buf, 256, L"%s\0", (GPRMC.active != -1) ? GPRMC.active ? L"active" : L"void" : L"unknown");
		wstrcpy(master + offset, buf);
		offset += ACTIVE_MAX_LEN + SPACES;
		swprintf_s(buf, 256, L"%d°\0", GPRMC.latitude.deg);
		wstrcpy(master + offset, buf);
		offset += DEGREES_MAX_LEN + SINGLE_SPACE;
		swprintf_s(buf, 256, L"%f\0", GPRMC.latitude.min);
		wstrcpy(master + offset, buf);
		offset += GENERIC_DOUBLE_MAX_LEN + SINGLE_SPACE;
		swprintf_s(buf, 256, L"%c\0", GPRMC.latitude.dir);
		wstrcpy(master + offset, buf);
		offset += CHAR_MAX_LEN + SPACES;
		swprintf_s(buf, 256, L"%d°\0", GPRMC.longitude.deg);
		wstrcpy(master + offset, buf);
		offset += DEGREES_MAX_LEN + SINGLE_SPACE;
		swprintf_s(buf, 256, L"%f\0", GPRMC.longitude.min);
		wstrcpy(master + offset, buf);
		offset += GENERIC_DOUBLE_MAX_LEN + SINGLE_SPACE;
		swprintf_s(buf, 256, L"%c\0", GPRMC.longitude.dir);
		wstrcpy(master + offset, buf);
		offset += CHAR_MAX_LEN + SPACES;
		swprintf_s(buf, 256, L"%f\0", GPRMC.v);
		wstrcpy(master + offset, buf);
		offset += GENERIC_DOUBLE_MAX_LEN + SPACES;
		swprintf_s(buf, 256, L"%f\0", GPRMC.ta);
		wstrcpy(master + offset, buf);
		offset += GENERIC_DOUBLE_MAX_LEN + SPACES;
		swprintf_s(buf, 256, L"%d\0", GPRMC.d_d);
		wstrcpy(master + offset, buf);
		offset += DT_MAX_LEN + SINGLE_SPACE;
		swprintf_s(buf, 256, L"%d\0", GPRMC.d_m);
		wstrcpy(master + offset, buf);
		offset += DT_MAX_LEN + SINGLE_SPACE;
		swprintf_s(buf, 256, L"%d\0", GPRMC.d_y);
		wstrcpy(master + offset, buf);
		offset += DT_MAX_LEN + SPACES;
		swprintf_s(buf, 256, L"%f\0", GPRMC.mv);
		wstrcpy(master + offset, buf);
		offset += GENERIC_DOUBLE_MAX_LEN + SPACES;
		swprintf_s(buf, 256, L"%c\0", GPRMC.mv_dir);
		wstrcpy(master + offset, buf);
		offset += CHAR_MAX_LEN + SPACES;
		swprintf_s(buf, 256, L"%c\0", GPRMC.mode);
		wstrcpy(master + offset, buf);
		printHL(7);

		offset = WIDTH * 9;
		swprintf_s(buf, 256, L"Waypoints created: %d\0", (next_open != nullptr) ? next_open - waypoints : last_waypoint_slot - waypoints);
		wstrcpy(master + offset, buf);
		offset += WIDTH;
		swprintf_s(buf, 256, L"Route build status: %s\0", (distances == nullptr) ? L"not built" : L"built");
		wstrcpy(master + offset, buf);

		offset += WIDTH * 2;
		uint old = content_page_index;
		content_page_index = 0;
		for (uint i = 0; i < last_waypoint_slot - waypoints; i++) {

			if (waypoints[i] != NULL) {
				
				swprintf_s(buf, 256, L"Waypoint %d: \0", i + 1); //len 13
				wstrcpy(screens[content_page_index] + offset, buf);
				offset += 13;
				swprintf_s(buf, 256, L"%d°\0", waypoints[i]->latitude->deg);
				wstrcpy(screens[content_page_index] + offset, buf);
				offset += DEGREES_MAX_LEN + SINGLE_SPACE;
				swprintf_s(buf, 256, L"%f\0", waypoints[i]->latitude->min);
				wstrcpy(screens[content_page_index] + offset, buf);
				offset += GENERIC_DOUBLE_MAX_LEN + SINGLE_SPACE;
				swprintf_s(buf, 256, L"%c\0", waypoints[i]->latitude->dir);
				wstrcpy(screens[content_page_index] + offset, buf);
				offset += CHAR_MAX_LEN + SPACES;
				swprintf_s(buf, 256, L"%d°\0", waypoints[i]->latitude->deg);
				wstrcpy(screens[content_page_index] + offset, buf);
				offset += DEGREES_MAX_LEN + SINGLE_SPACE;
				swprintf_s(buf, 256, L"%f\0", waypoints[i]->latitude->min);
				wstrcpy(screens[content_page_index] + offset, buf);
				offset += GENERIC_DOUBLE_MAX_LEN + SINGLE_SPACE;
				swprintf_s(buf, 256, L"%c\0", waypoints[i]->latitude->dir);
				wstrcpy(screens[content_page_index] + offset, buf);
				offset += CHAR_MAX_LEN + SPACES;
				swprintf_s(buf, 256, L"%f\0", waypoints[i]->rel_x);
				wstrcpy(screens[content_page_index] + offset, buf);
				offset += GENERIC_DOUBLE_MAX_LEN + SPACES;
				swprintf_s(buf, 256, L"%f\0", waypoints[i]->rel_y);
				wstrcpy(screens[content_page_index] + offset, buf);

				offset = CONTENT_START + WIDTH * (i % 30) + WIDTH / 2;
				if (distances != nullptr) {

					swprintf_s(buf, 256, L"Distance from waypoint %d to %d: %f\0", i + 1, (i + 2 != last_distance_slot - distances + 2) ? i + 2 : 1, distances[i]);
				}
				else swprintf_s(buf, 256, L"Distances are not calculated yet!  Press 'b' to build.\0");
				wstrcpy(screens[content_page_index] + offset, buf);
				offset += WIDTH / 2;
				//if we need to go to another screen
				if (offset >= CONTENT_STOP) {

					//if we need to create a new page, rather than just switch pages
					if (content_page_index + 1 >= num_content_pages) {

						//"expand" screens for one more
						wchar_t** new_screens = (wchar_t**)malloc((num_content_pages + 1) * sizeof(wchar_t*));
						if (new_screens == NULL) {

							fprintf(fp, "[%s] FATAL: ERROR | At function: \"%s\", line: %d | MALLOC FAILED CREATING new_screens (plural)!\n", current_time().c_str(), __FUNCTION__, __LINE__);
							return false;
						}
						for (uint i = 0; i < num_content_pages + 1; i++) new_screens[i] = nullptr;
						for (uint i = 0; i < num_content_pages; i++) new_screens[i] = screens[i];
						free(screens);
						screens = new_screens;

						//create new screen
						wchar_t* new_screen = (wchar_t*)malloc(TOTAL_CHARS * sizeof(wchar_t));
						if (new_screen == NULL) {

							fprintf(fp, "[%s] FATAL: ERROR | At function: \"%s\", line: %d | MALLOC FAILED CREATING new_screen!\n", current_time().c_str(), __FUNCTION__, __LINE__);
							return false;
						}
						memset(new_screen, 0, TOTAL_CHARS * sizeof(wchar_t));
						screens[num_content_pages] = new_screen;
						num_content_pages++;
					}

					offset = CONTENT_START; //go to start of content next page
					content_page_index++;
				}
			}
		}
		content_page_index = old;

		/*
		The different console pages are just copied to the content portion of
		the first master, and then the first master is shown.  We should only
		copy if the current active master is not the first one.
		START OF CONTENT (inclusive) = master[0] + WIDTH * 9;
		END OF CONTENT (exclusive) = master[0] + WIDTH * (HEIGHT - 2);
		*/

		for (uint i = 0; i < CONTENT_STOP - CONTENT_START; i++) {

			master[i + CONTENT_START] = screens[content_page_index][i + CONTENT_START];
		}

		//fprintf(fp, "[%s] %d %d\n", current_time().c_str(), content_page_index, num_content_pages);
		WriteConsoleOutputCharacter(hConsole, master, TOTAL_CHARS, { 0, 0 }, & dwBytesWritten);
	}

	FLAGS ^= FLAG_END;

	fprintf(fp, "[%s] Cleaning up...\n", current_time().c_str());

	free(waypoints);

	fprintf(fp, "[%s] Waiting for serial thread to rejoin...\n", current_time().c_str());
	serial_thread.join();
	fprintf(fp, "[%s] Serial thread has rejoined\n", current_time().c_str());

	fprintf(fp, "[%s] Closing log file...", current_time().c_str());
	fclose(fp);

	return 0;
}

void serial_main() {

#ifndef USE_FAKE_DATA
	const char* PORT = "COM8";
	SerialPort ardi(L"COM8", true, false);
	if (ardi.Setup()) fprintf(fp, "[%s] Successfully set up serial port on %s!\n", current_time().c_str(), PORT);
	else {

		fprintf(fp, "[%s] FATAL ERROR | COULD NOT SETUP ARDUINO: %d\n", current_time().c_str(), GetLastError());
		FLAGS ^= FLAG_SERIAL_END;
	}
	uint bytesRead = 0;
#else
	//======================================TESTING======================================================
	//const char* src = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\n\x04";
	const char* src = "$GPRMC,,,,,,,,00000000.123456789,,,*00\n\x04";
	uint len = strlen(src);
	uchar* test = (uchar*)malloc(sizeof(uchar) * len); //one less because it is a null terminated string
	if (test == NULL) {

		fprintf(fp, "[%s] ERROR | At function: \"%s\", line: %d | first byte must be a $\n", current_time().c_str(), __FUNCTION__, __LINE__);
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
		if (FLAGS & FLAG_DO_DEBUG) fprintf(fp, "[%s] %s\n", current_time().c_str(), buffer);
		uchar* ptr = buffer;
#else
		uchar* ptr = test;
#endif
		while (searching && (*ptr != 0x04)) {

			while (*ptr != 0x04 && *++ptr != 'G');
			if (*ptr == 0x04) break;
			else searching = !(/**++ptr == 'G' && */*++ptr == 'P' && *++ptr == 'R' && *++ptr == 'M' && *++ptr == 'C');
		}
		if (searching) fprintf(fp, "[%s] ERROR | At function: \"%s\", line: %d | COULD NOT FIND GPRMC IN BUFFER\n", current_time().c_str(), __FUNCTION__, __LINE__);
		else {

			ptr -= 5;
			if (valid(ptr, false)) {

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

		fprintf(fp, "[%s] ERROR | At function: \"%s\", line: %d | MALLOC FAILED WHEN CREATING WAYPOINT STRUCT\n", current_time().c_str(), __FUNCTION__, __LINE__);
		return nullptr;
	}

	Position* latitude = (Position*)malloc(sizeof(Position));
	if (latitude == NULL) {

		fprintf(fp, "[%s] ERROR | At function: \"%s\", line: %d | MALLOC FAILED WHEN CREATING LATITUDE STRUCT\n", current_time().c_str(), __FUNCTION__, __LINE__);
		return nullptr;
	}
	else {

		latitude->deg = GPRMC.latitude.deg;
		latitude->min = GPRMC.latitude.min;
		latitude->dir = GPRMC.latitude.dir;
	}

	Position* longitude = (Position*)malloc(sizeof(Position));
	if (longitude == NULL) {

		fprintf(fp, "[%s] ERROR | At function: \"%s\", line: %d | MALLOC FAILED WHEN CREATING LONGITUDE STRUCT\n", current_time().c_str(), __FUNCTION__, __LINE__);
		return nullptr;
	}
	else {

		longitude->deg = GPRMC.longitude.deg;
		longitude->min = GPRMC.longitude.min;
		longitude->dir = GPRMC.longitude.dir;
	}

	w->latitude = latitude;
	w->longitude = longitude;
	w->rel_x = 0;
	w->rel_y = 0;
	return w;
}

bool valid(uchar* buffer, const bool use_chksm = true) {
	//TODO: Check if more than one *
	uchar* ptr = buffer;
	if (*ptr != '$') {

		fprintf(fp, "[%s] ERROR | At function: \"%s\", line: %d | first byte must be a $\n", current_time().c_str(), __FUNCTION__, __LINE__);
		return false; //first byte must be a $
	}
	/*ptr += len - 1; //go to the last byte
	if (*ptr != '\n') {

		fprintf(fp, "[%s] ERROR | At function: \"%s\", line: %d | last byte must be a newline, tested was a %d\n", current_time().c_str(), __FUNCTION__, __LINE__, (int)*ptr);
		return false; //last byte must be a newline
	}
	ptr -= len - 1; //go back to $*/
	bool searching = true;
	while (searching && *++ptr != '\n') { searching = *ptr != '*'; } //make sure to not increment if we are done searching
	if (searching) {
		
		fprintf(fp, "[%s] ERROR | At function: \"%s\", line: %d | could not find *\n", current_time().c_str(), __FUNCTION__, __LINE__);
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

		fprintf(fp, "[%s] ERROR | At function: \"%s\", line: %d | the checksum is not valid; expected %d, got %d\n", current_time().c_str(), __FUNCTION__, __LINE__, checksum, acc);
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

void printGPRMC() {

	std::string t_buffer = current_time();

	fprintf(
		fp, 
		"[%s] %d %d %d    %s    %d° %f' %c    %d° %f' %c    %f    %f    %d %d %d    %f %c    %c\n",
		current_time().c_str(),
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

void printWaypoints() {

	for (uint i = 0; i < last_waypoint_slot - waypoints; i++) {

		if (waypoints[i] != NULL) {

			fprintf(fp, 
				"[%s] Waypoint %d: %d %f %c    %d %f %c        %f    %f\n",
				current_time().c_str(),
				i + 1,
				waypoints[i]->latitude->deg, waypoints[i]->latitude->min, waypoints[i]->latitude->dir,
				waypoints[i]->longitude->deg, waypoints[i]->longitude->min, waypoints[i]->longitude->dir,
				waypoints[i]->rel_x,
				waypoints[i]->rel_y
			);
		}
		else fprintf(fp, "[%s] Waypoint: %d: NULL\n", current_time().c_str(), i + 1);
	}
	if (distances == nullptr) fprintf(fp, "[%s] Distances are still null\n", current_time().c_str());
	else {

		for (uint i = 0; i < last_distance_slot - distances + 1; i++) {

			fprintf(fp, 
				"[%s] Distance from waypoint %d to %d: %f\n",
				current_time().c_str(),
				i + 1,
				(i + 2 != last_distance_slot - distances + 2) ? i + 2 : 1,
				distances[i]
			);
		}
	}
}

void clear_pages() {

	//fprintf(fp, "[%s] %d\n", current_time().c_str(), content_page_index);
	memset(master, 0, TOTAL_CHARS * sizeof(wchar_t));
  	for (uint i = 0; i < num_content_pages; i++) memset(screens[i], 0, TOTAL_CHARS * sizeof(wchar_t));
}

void wstrcpy(wchar_t* dest, wchar_t* src) {

	//DOES NOT PUT NULL TERMINATOR IN DEST!
 	while (*src != L'\0') *dest++ = *src++;
}

void printHL(uint row) {

	for (uint i = WIDTH * row; i < WIDTH * (row + 1) - 1; i++) master[i] = FULL;
}

void clearLine(uint row) {

	for (uint i = WIDTH * row; i < WIDTH * (row + 1) - 1; i++) master[i] = ' ';
}

std::string current_time() {

	/*auto now = std::chrono::system_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;
	auto timer = std::chrono::system_clock::to_time_t(now);
	std::tm* bt = (std::tm*)malloc(sizeof(std::tm));
	if (bt == NULL) {

		fprintf(fp, "[%s] ERROR | At function: \"%s\", line: %d | MALLOC FAILED WHEN CREATING bt\n", current_time().c_str(), __FUNCTION__, __LINE__);
		return "INVALID";
	}
	localtime_s(bt, &timer);
	std::ostringstream oss;
	oss << std::put_time(bt, "%H:%M:%S");
	oss << '.' << std::setfill('0') << std::setw(6) << ms.count();
	std::string s = oss.str();*/
	return std::string("IDK");
}
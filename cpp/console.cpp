#include "console.h"

extern Waypoint** waypoints;
extern Waypoint** next_open;
extern Waypoint** last_waypoint_slot;
extern GPRMC_STRUCT GPRMC;
extern double* distances;
extern double* last_distance_slot;
extern FILE* fp;

wchar_t** screens = (wchar_t**)malloc(sizeof(wchar_t*)); //only one to start
wchar_t* master = (wchar_t*)malloc(TOTAL_CHARS * sizeof(wchar_t)); //master buffer
wchar_t* map = (wchar_t*)malloc(TOTAL_CHARS * sizeof(wchar_t));
wchar_t* page_one = (wchar_t*)malloc(TOTAL_CHARS * sizeof(wchar_t)); //page one
HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, NULL, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
DWORD dwBytesWritten = 0;
wchar_t buf[256];

uint content_page_index = 1;
uint num_content_pages = 2;

double least_x = 0;
double greatest_x = 0;
double least_y = 0;
double greatest_y = 0;
uint frame_width = 0;
uint frame_height = 0;
int x_offset = 0;
int y_offset = 0;
double x_bounds = 0.0;
double y_bounds = 0.0;
bool longer = false;
uint x_multiplier = 0; //when in proportion to height, the font size makes the scale be 1:2, not 1:1

bool console_init() {

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
	if (map == NULL) {

		fprintf(fp, "[%s] FATAL: ERROR | At function: \"%s\", line: %d | MALLOC FAILED CREATING map!\n", current_time().c_str(), __FUNCTION__, __LINE__);
		return false;
	}

	screens[0] = map;
	screens[1] = page_one;
	SetConsoleActiveScreenBuffer(hConsole);

	return true;
}

bool update() {

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
		(content_page_index + 1 < num_content_pages) ? L"---->" : L"     "
	);
	wstrcpy(master + WIDTH * (HEIGHT - 1), buf);
	//SetPixel(hConsole, )

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

	offset = CONTENT_START;
	uint old = content_page_index;
	content_page_index = 1;
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
			swprintf_s(buf, 256, L"%f\0", waypoints[i]->global_rel_x);
			wstrcpy(screens[content_page_index] + offset, buf);
			offset += GENERIC_DOUBLE_MAX_LEN + SPACES;
			swprintf_s(buf, 256, L"%f\0", waypoints[i]->global_rel_y);
			wstrcpy(screens[content_page_index] + offset, buf);
			offset += GENERIC_DOUBLE_MAX_LEN + SPACES;
			swprintf_s(buf, 256, L"%f\0", waypoints[i]->local_rel_x);
			wstrcpy(screens[content_page_index] + offset, buf);
			offset += GENERIC_DOUBLE_MAX_LEN + SPACES;
			swprintf_s(buf, 256, L"%f\0", waypoints[i]->local_rel_y);
			wstrcpy(screens[content_page_index] + offset, buf);
			offset += GENERIC_DOUBLE_MAX_LEN + SPACES;

			//offset = CONTENT_START + WIDTH * (i % 30) + WIDTH / 2;
			if (distances != nullptr) {

				swprintf_s(buf, 256, L"Distance from waypoint %d to %d: %f\0", i + 1, (i + 2 != last_distance_slot - distances + 2) ? i + 2 : 1, distances[i]);
			}
			else swprintf_s(buf, 256, L"Distances are not calculated yet!  Press 'b' to build.\0");
			wstrcpy(screens[content_page_index] + offset, buf);
			offset = CONTENT_START + WIDTH * (i + 1);
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
					for (uint i = 0; i < TOTAL_CHARS; i++) new_screen[i] = 0;
					screens[num_content_pages] = new_screen;
					num_content_pages++;
				}

				offset = CONTENT_START; //go to start of content next page
				content_page_index++;
			}
		}
	}
	content_page_index = old;

	//MAP
	swprintf_s(buf, 256, L"                                                                                                  Map                                                                                                   \0");
	wstrcpy(screens[0] + WIDTH * 1, buf);

	//top
	for (uint i = 0; i < MAX_MAP_WIDTH + 4; i++) {

		screens[0][((HEIGHT - MAX_MAP_HEIGHT) / 2 - 2) * WIDTH + i + (WIDTH - MAX_MAP_WIDTH) / 2 - 2] = FULL;
	}
	//left
	for (uint i = 0; i < MAX_MAP_HEIGHT + 4; i++) {

		screens[0][(i + (HEIGHT - MAX_MAP_HEIGHT) / 2 - 2) * WIDTH + (WIDTH - MAX_MAP_WIDTH) / 2 - 2] = FULL;
	}
	//bottom
	for (uint i = 0; i < MAX_MAP_WIDTH + 4; i++) {

		screens[0][(HEIGHT - ((HEIGHT - MAX_MAP_HEIGHT) / 2 - 2)) * WIDTH + i + (WIDTH - MAX_MAP_WIDTH) / 2 - 2] = FULL;
	}
	//right
	for (uint i = 0; i < MAX_MAP_HEIGHT + 4; i++) {

		screens[0][(i + (HEIGHT - MAX_MAP_HEIGHT) / 2 - 2) * WIDTH + WIDTH - ((WIDTH - MAX_MAP_WIDTH) / 2 - 2)] = FULL;
	}

	//if there are at least two waypoints available, make a map out of it
	if (distances != nullptr) {

		for (uint i = 0; i < last_waypoint_slot - waypoints; i++) {

			//it looks backwards, but the greatest number is the lowest number
			if (waypoints[i]->global_rel_x < greatest_x) greatest_x = waypoints[i]->global_rel_x;
			if (waypoints[i]->global_rel_x > least_x) least_x = waypoints[i]->global_rel_x;
			if (waypoints[i]->global_rel_y < greatest_y) greatest_y = waypoints[i]->global_rel_y;
			if (waypoints[i]->global_rel_y > least_y) least_y = waypoints[i]->global_rel_y;
		}

		//some more backwards nonesense :)
		x_bounds = -greatest_x + least_x;
		y_bounds = -greatest_y + least_y;
		longer = x_bounds / y_bounds >= MAP_AR;
		x_multiplier = longer ? 1 : 2; //when in proportion to height, the font size makes the scale be 1:2, not 1:1
		
		if (longer) {

			frame_width = MAX_MAP_WIDTH;
			frame_height = RoundLit(MAX_MAP_WIDTH * y_bounds / x_bounds);
		}
		else {

			frame_width = RoundLit(MAX_MAP_HEIGHT * x_bounds / y_bounds);
			frame_height = MAX_MAP_HEIGHT;
		}

		x_offset = (WIDTH - frame_width * x_multiplier) / 2;
		y_offset = (HEIGHT - frame_height) / 2;

		for (uint i = 0; i < last_waypoint_slot - waypoints; i++) {

			plot(waypoints[i]->global_rel_x, waypoints[i]->global_rel_y, FULL);
		}

		//after plotting the waypoints, draw lines in between the distances
		if (distances != nullptr) {

			double slope = 0.0;
			double x_diff = 0.0;
			double y_diff = 0.0;
			double x = 0.0;
			double y = 0.0;
			char c = 'o';
			for (uint i = 0; i < last_waypoint_slot - waypoints; i++) {

				int index_of_next = (i + 1 != last_waypoint_slot - waypoints) ? i + 1 : 0;
				x_diff = waypoints[index_of_next]->global_rel_x - waypoints[i]->global_rel_x;
				y_diff = waypoints[index_of_next]->global_rel_y - waypoints[i]->global_rel_y;
				slope = y_diff / x_diff;
				for (int j = 0; j < 200; j++) {

					if (waypoints[i]->global_rel_x > waypoints[index_of_next]->global_rel_x) { //if we going right
						
						x = waypoints[i]->global_rel_x - (j + 1) * abs(x_diff) / 200;
					}
					else {

						x = waypoints[i]->global_rel_x + (j + 1) * abs(x_diff) / 200;
					}
					y = slope * (x - waypoints[i]->global_rel_x) + waypoints[i]->global_rel_y;
					//-\|/
					if (slope < 0) {

						if (slope >= -.5) c = '-';
						else if (slope > -2) c = '/';
						else c = '|';
					}
					else {

						if (slope <= .5) c = '-';
						else if (slope < 2) c = '\\';
						else c = c = '|';
					}
					plot(x, y, c);
				}
			}
		}
	}

	/*
	The different console pages are just copied to the content portion of
	the first master, and then the first master is shown.  We should only
	copy if the current active master is not the first one.
	START OF CONTENT (inclusive) = master[0] + WIDTH * 9;
	END OF CONTENT (exclusive) = master[0] + WIDTH * (HEIGHT - 2);
	*/

	if (content_page_index != 0) {

		for (uint i = 0; i < CONTENT_STOP - CONTENT_START; i++) {

			master[i + CONTENT_START] = screens[content_page_index][i + CONTENT_START];
		}
	}
	else for (uint i = 0; i < TOTAL_CHARS; i++) master[i] = screens[0][i];

	//fprintf(fp, "[%s] %d %d\n", current_time().c_str(), content_page_index, num_content_pages);
	WriteConsoleOutputCharacter(hConsole, master, TOTAL_CHARS, { 0, 0 }, &dwBytesWritten);
	return true;
}

void printHL(uint row) {

	for (uint i = WIDTH * row; i < WIDTH * (row + 1) - 1; i++) master[i] = FULL;
}

void clearLine(uint row) {

	for (uint i = WIDTH * row; i < WIDTH * (row + 1) - 1; i++) master[i] = ' ';
}

void wstrcpy(wchar_t* dest, wchar_t* src) {

	//DOES NOT PUT NULL TERMINATOR IN DEST!
	while (*src != L'\0') *dest++ = *src++;
}

void clear_pages() {

	//fprintf(fp, "[%s] %d\n", current_time().c_str(), content_page_index);
	for (uint i = 0; i < TOTAL_CHARS; i++) master[i] = 0;
	for (uint i = 0; i < num_content_pages; i++) {

		for (uint j = 0; j < TOTAL_CHARS; j++) screens[i][j] = 0;
	}
}

void plot(double x, double y, int c) {

	int x_coord = 0;
	int y_coord = 0;

	if (x < 0) {

		//+ bc its negative
		x_coord = RoundLit((-x + least_x) * frame_width / x_bounds);
	}
	else {

		x_coord = RoundLit((least_x - x) * frame_width / x_bounds);
	}
	if (y < 0) {

		y_coord = RoundLit((-y + least_y) * frame_height / y_bounds);
	}
	else {

		y_coord = RoundLit((least_y - y) * frame_height / y_bounds);
	}

	if (screens[0][(y_coord + y_offset) * WIDTH + x_coord * x_multiplier + x_offset] != FULL) screens[0][(y_coord + y_offset) * WIDTH + x_coord * x_multiplier + x_offset] = c;
}
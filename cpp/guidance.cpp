#include "guidance.h"

extern FILE* fp;

GPRMC_STRUCT GPRMC;

Waypoint** waypoints = (Waypoint**)malloc(NUM_INITIAL_WAYPOINT * sizeof(Waypoint*)); //array of Waypoint pointers
Waypoint** next_open = waypoints; //pointer to next open slot
Waypoint** last_waypoint_slot = waypoints + NUM_INITIAL_WAYPOINT - 1; //pointer to last slot in array

double* distances = nullptr;
double* last_distance_slot = nullptr;

bool guidance_init() {

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
	
	return true;
}

void guidance_cleanup() {

	free(waypoints);
}

bool add_waypoint() {

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
	
	return true;
}

bool build() {

	fprintf(fp, "[%s] Building route...\n", current_time().c_str());
	if (next_open - waypoints == 0) {

		fprintf(fp, "[%s] ERROR | At function: \"%s\", line: %d | Cannot build route if there are no waypoints!\n", current_time().c_str(), __FUNCTION__, __LINE__);
		return false;
	}
	//first, shave nulls
	uint acc = 0; //find number of open slots
	for (int i = 0; i < last_waypoint_slot - waypoints; i++) if (waypoints[i] == NULL) acc++;

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
	
	return true;
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

double sq(double x) { return x * x; }
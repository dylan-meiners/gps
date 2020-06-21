#include "guidance.h"

extern FILE* fp;
extern bool executing;

GPRMC_STRUCT GPRMC;

Waypoint** waypoints = (Waypoint**)malloc(NUM_INITIAL_WAYPOINT * sizeof(Waypoint*)); //array of Waypoint pointers
Waypoint** next_open = waypoints; //pointer to next open slot
Waypoint** last_waypoint_slot = waypoints + NUM_INITIAL_WAYPOINT - 1; //pointer to last slot in array

double* distances = nullptr;
double* last_distance_slot = nullptr;

wchar_t dbg_buf[256] = { 0 };

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
			else {

				//if there is at least one point
				if (waypoints[0] != NULL) {

					bool same_exists = false;
					for (uint i = 0; i < next_open - waypoints && !same_exists; i++) {

						same_exists =
							(waypoints[i]->latitude->deg == w->latitude->deg && waypoints[i]->latitude->min == w->latitude->min && waypoints[i]->latitude->dir == w->latitude->dir) &&
							(waypoints[i]->longitude->deg == w->longitude->deg && waypoints[i]->longitude->min == w->longitude->min && waypoints[i]->longitude->dir == w->longitude->dir);
					}
					if (same_exists) {

						fprintf(fp, "[%s] ERROR | At function: \"%s\", line: %d | COULD NOT CREATE A NEW WAYPOINT BECUASE IT ALREADY EXISTS!\n", current_time().c_str(), __FUNCTION__, __LINE__);
						free(w);
						w = nullptr;
					}
				}
				if (w != nullptr) {
				
					*next_open = w;
					next_open++;
					fprintf(fp,
						"[%s] Successfully created a new waypoint at curent location: %d %f %c    %d %f %c\n",
						current_time().c_str(),
						w->latitude->deg, w->latitude->min, w->latitude->dir,
						w->longitude->deg, w->longitude->min, w->longitude->dir
					);
				}
			}
		}
	}
	else fprintf(fp, "[%s] ERROR | At function: \"%s\", line: %d | CANNOT CREATE NEW WAYPOINT BECAUSE THERE IS NO CURRENT DATA!\n", current_time().c_str(), __FUNCTION__, __LINE__);
	fprintf(fp, "[%s] Finished attempting to create a new waypoint.\n", current_time().c_str());
#else
	fprintf(fp, "[%s] Attempting to create new waypoints from fake data...\n", current_time().c_str());
	if (waypoints[0] == NULL) {

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
		//la2->min = 0.826166667;
		la2->dir = 'N';
		lo2->deg = 91;
		lo2->min = 18.281166667;
		//lo2->min = 10.797666667;
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
		w1->global_rel_x = 0;
		w1->global_rel_y = 0;
		w1->local_rel_x = 0;
		w1->local_rel_y = 0;

		w2->latitude = la2;
		w2->longitude = lo2;
		w2->global_rel_x = 0;
		w2->global_rel_y = 0;
		w2->local_rel_x = 0;
		w2->local_rel_y = 0;

		w3->latitude = la3;
		w3->longitude = lo3;
		w3->global_rel_x = 0;
		w3->global_rel_y = 0;
		w3->local_rel_x = 0;
		w3->local_rel_y = 0;

		w4->latitude = la4;
		w4->longitude = lo4;
		w4->global_rel_x = 0;
		w4->global_rel_y = 0;
		w4->local_rel_x = 0;
		w4->local_rel_y = 0;

		waypoints[0] = w1;
		waypoints[1] = w2;
		waypoints[2] = w3;
		waypoints[3] = w4;

		next_open += 4;
		fprintf(fp, "[%s] Finished attempting to create new waypoints from fake data.\n", current_time().c_str());
		printWaypoints();
	}
	else fprintf(fp, "[%s] ERROR | At function: \"%s\", line: %d | COULD NOT CREATE FAKE WAYPOINTS BECUASE THEY ALREADY EXIST!\n", current_time().c_str(), __FUNCTION__, __LINE__);
#endif

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
	for (uint i = 0; i < num_new_slots; i++) { //find relative distances for the waypoints, based on the first one being (0, 0)

		waypoints[i]->global_rel_x = (double)waypoints[i]->longitude->deg * LONG_DEG_FEET + waypoints[i]->longitude->min * LONG_MIN_TO_FEET_MULTIPLIER;
		waypoints[i]->global_rel_y = (double)waypoints[i]->latitude->deg * LAT_DEG_FEET + waypoints[i]->latitude->min * LAT_MIN_TO_FEET_MULTIPLIER;
	}
	//then, find the global relative values
	for (uint i = num_new_slots - 1; i > 0; i--) {

		waypoints[i]->global_rel_x -= waypoints[0]->global_rel_x;
		waypoints[i]->global_rel_y -= waypoints[0]->global_rel_y;
	}
	waypoints[0]->global_rel_x = 0.0;
	waypoints[0]->global_rel_y = 0.0;
	//lastly, find the local relative values
	for (uint i = num_new_slots - 1; i > 0; i--) {

		waypoints[i]->local_rel_x = waypoints[i]->global_rel_x - waypoints[i - 1]->global_rel_x;
		waypoints[i]->local_rel_y = waypoints[i]->global_rel_y - waypoints[i - 1]->global_rel_y;
	}

	distances = (double*)malloc(num_new_slots * sizeof(double)); //now, create an array of distances in between the waypoints
	for (uint i = 0; i < num_new_slots; i++) distances[i] = 0.0;
	if (distances == NULL) {

		fprintf(fp, "[%s] FATAL: ERROR | At function: \"%s\", line: %d | MALLOC FAILED CREATING distances!\n", current_time().c_str(), __FUNCTION__, __LINE__);
		return false;
	}
	for (uint i = 0; i < num_new_slots; i++) {

		distances[i] = sqrt(

			sq(waypoints[(i + 1 != num_new_slots) ? i + 1 : 0]->global_rel_x - waypoints[i]->global_rel_x) +
			sq(waypoints[(i + 1 != num_new_slots) ? i + 1 : 0]->global_rel_y - waypoints[i]->global_rel_y)
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

double slope(Waypoint* one, Waypoint* two) {

	/*int index_of_next = (i + 1 != last_waypoint_slot - waypoints) ? i + 1 : 0;
	x_diff = waypoints[index_of_next]->global_rel_x - waypoints[i]->global_rel_x;
	y_diff = waypoints[index_of_next]->global_rel_y - waypoints[i]->global_rel_y;
	return x_diff / y_diff;*/
	return -1.0;
}

//this f() is the big one
//TODO: CHECK IF NO DATA
void step() {

	static double distance = 0.0;
	double dist_to_path = 0.0;
	uint index_of_distance = 0;
	uint index_of_next_distance = 0;
#ifndef USE_FAKE_DATA
	Waypoint* current = new_current_waypoint();
#else
	Waypoint* current = (Waypoint*)malloc(sizeof(Waypoint));
	Position* lat = (Position*)malloc(sizeof(Position));
	Position* lon = (Position*)malloc(sizeof(Position));
	if (current == NULL || lat == NULL || lon == NULL) {

		fprintf(fp, "[%s] ERROR | At function: \"%s\", line: %d | MALLOC FAILED WHEN CREATING WAYPOINT STRUCT, OR POSITION STRUCT\n", current_time().c_str(), __FUNCTION__, __LINE__);
	}
	lat->deg = 46;
	lat->min = 0.217666667;
	lat->dir = 'N';
	lon->deg = 91;
	lon->min = 18.224666667;
	lon->dir = 'W';
	current->latitude = lat;
	current->longitude = lon;
#endif
	current->global_rel_x =
		((double)current->longitude->deg * LONG_DEG_FEET + current->longitude->min * LONG_MIN_TO_FEET_MULTIPLIER)
		-
		((double)waypoints[0]->longitude->deg * LONG_DEG_FEET + waypoints[0]->longitude->min * LONG_MIN_TO_FEET_MULTIPLIER);
	current->global_rel_y =
		((double)current->latitude->deg * LAT_DEG_FEET + current->latitude->min * LAT_MIN_TO_FEET_MULTIPLIER)
		-
		((double)waypoints[0]->latitude->deg * LAT_DEG_FEET + waypoints[0]->latitude->min * LAT_MIN_TO_FEET_MULTIPLIER);

	bool searching = true;
	double total_dist = 0.0;
	for (uint i = 0; i < last_distance_slot - distances && searching; i++) {

		total_dist += distances[i];
		if (distance < total_dist) {

			index_of_distance = i;
			index_of_next_distance = i < last_distance_slot - distances - 1 ? i + 1 : 0;
			searching = false;
		}
	}

	double slope =
		(waypoints[index_of_next_distance]->global_rel_y - waypoints[index_of_distance]->global_rel_y)
		/
		(waypoints[index_of_next_distance]->global_rel_x - waypoints[index_of_distance]->global_rel_x);
	double int_x = 0.0;
	double int_y = 0.0;
	//I solved a system of equations for the equation of the path, and the equation of the current waypoint with slope perpendicular to the path.
	//The equation of the current wayopint is solved for x then substituded into the path's equation
	int_y =
		(-slope * waypoints[index_of_distance]->global_rel_x + waypoints[index_of_distance]->global_rel_y + slope * current->global_rel_x + sq(slope) * current->global_rel_y)
		/
		(sq(slope) + 1);
	int_x = (int_y + slope * waypoints[index_of_distance]->global_rel_x - waypoints[index_of_distance]->global_rel_y) / slope;
	//we then find the distance between the current point and the path
	dist_to_path = sqrt(sq(int_x - current->global_rel_x) + sq(int_y - current->global_rel_y));
	double target_distance = 0.0;
	if (dist_to_path <= HANDS_OFF_ZONE) {

		searching = true;
		total_dist = 0.0;
		target_distance = distance + THROW_DISTANCE;
	}

	for (uint i = 0; i < last_distance_slot - distances && searching; i++) {

		total_dist += distances[i];
		if (target_distance < total_dist) {

			index_of_distance = i;
			searching = false;
		}
	}

	slope =
		(waypoints[index_of_next_distance]->global_rel_y - waypoints[index_of_distance]->global_rel_y)
		/
		(waypoints[index_of_next_distance]->global_rel_x - waypoints[index_of_distance]->global_rel_x);
	double target_x = 0.0;
	double target_y = 0.0;

	double angle = atan(slope);
	target_x = (target_distance - (index_of_distance != 0 ? distances[index_of_distance] : 0)) * cos(angle) + waypoints[index_of_distance]->global_rel_x;
	target_y = (target_distance - (index_of_distance != 0 ? distances[index_of_distance] : 0)) * sin(angle) + waypoints[index_of_distance]->global_rel_y;

	angle = atan((current->global_rel_y - target_y) / (current->global_rel_x - target_x)) * 180.0 / M_PI;
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
	w->global_rel_x = 0;
	w->global_rel_y = 0;
	w->local_rel_x = 0;
	w->local_rel_y = 0;
	return w;
}

double sq(double x) { return x * x; }
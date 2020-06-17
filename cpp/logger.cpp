#include "logger.h"

FILE* fp;
extern GPRMC_STRUCT GPRMC;
extern Waypoint** waypoints;
extern Waypoint** last_waypoint_slot;
extern double* distances;
extern double* last_distance_slot;

void logger_init() {

	fopen_s(&fp, "o.txt", "w");
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
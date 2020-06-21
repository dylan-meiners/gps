#include <locale.h> //° symbol
#include <thread>
#include "typedefs.h"
#include <conio.h>
#include "ks.h"
#include "timing.h"
#include "SerialThread.h"
#include "console.h"
#include "logger.h"
#include "guidance.h"

/*
IMPORTANT - This code only for small distance applications.  It "converts"
			longitude and latitude to flat "coordinates".  The conversion units are
			LAT_DEG_FEET and LONG_DEG_FEET, which means the number of feet in one degree
			of latitude or longitude, respectively.

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

uint FLAGS = 0;
bool running = true;
bool executing = false;

extern FILE* fp;
extern uint content_page_index;
extern uint num_content_pages;

char ch;

int main() {

	logger_init();
	console_init();
	guidance_init();
	setlocale(LC_ALL, "");

	std::thread serial_thread(serial_main);
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
			e			Execute the route

			else		Stop executing
			*/

			ch = _getch();
			switch (ch) {

			case ' ':
				add_waypoint();
				break;

			case 'b': //b
				build();
				break;

			case 'q': //q
				running = false;
				break;

			case 't': //t
				FLAGS ^= FLAG_DO_DEBUG;
				if (FLAGS & FLAG_DO_DEBUG) fprintf(fp, "[%s] Enabled debug mode\n", current_time().c_str());
				else fprintf(fp, "[%s] Disabled debug mode\n", current_time().c_str());
				break;

			case -32: { //start of escape sequence for _getch()

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

			case 'e':
				executing = true;
				break;

			default:
				executing = false;
				break;
			}
		}

		if (executing) step();
		
		if (!update()) fprintf(fp, "[%s] ERROR | At function: \"%s\", line: %d | Console update failed!\n", current_time().c_str(), __FUNCTION__, __LINE__);

		if (FLAGS & FLAG_SERIAL_END) {

			running = false;
			fprintf(fp, "[%s] SERIAL THREAD WANTS TO END PREMATURELY, EXITING!\n", current_time().c_str());
		}
	}

	FLAGS ^= FLAG_END;

	fprintf(fp, "[%s] Cleaning up...\n", current_time().c_str());

	guidance_cleanup();

	fprintf(fp, "[%s] Waiting for serial thread to rejoin...\n", current_time().c_str());
	serial_thread.join();
	fprintf(fp, "[%s] Serial thread has rejoined\n", current_time().c_str());

	fprintf(fp, "[%s] Closing log file...", current_time().c_str());
	fclose(fp);

	return 0;
}
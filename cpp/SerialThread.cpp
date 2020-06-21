#include "SerialThread.h"

extern GPRMC_STRUCT GPRMC;
extern uint FLAGS;
extern FILE* fp;

uchar buffer[1024] = { 0 };

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

					GPRMC.t_h = (int)uchar_star_to_uint(ptr, 2);
					ptr += 2;
					GPRMC.t_m = (int)uchar_star_to_uint(ptr, 2);
					ptr += 2;
					GPRMC.t_s = (int)uchar_star_to_uint(ptr, 2);

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
					GPRMC.latitude.deg = (int)uchar_star_to_uint(ptr, acc);
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
					GPRMC.longitude.deg = (int)uchar_star_to_uint(ptr, acc);
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

					GPRMC.d_d = (int)uchar_star_to_uint(ptr, 2);
					ptr += 2;
					GPRMC.d_m = (int)uchar_star_to_uint(ptr, 2);
					ptr += 2;
					GPRMC.d_y = (int)uchar_star_to_uint(ptr, 2);

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

		//for (uint i = 0; i < 1024; i++) buffer[i] = 0;
		searching = true;
	}

#ifdef USE_FAKE_DATA
	free(test);
#endif
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
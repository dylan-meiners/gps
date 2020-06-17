#include "timing.h"

extern FILE* fp;

std::string current_time() {

	auto now = std::chrono::system_clock::now();
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
	std::string s = oss.str();
	return s;
}
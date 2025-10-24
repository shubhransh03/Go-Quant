#include "utils/time_utils.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#define HAVE_GMTIME_S
#endif

namespace utils {

std::string to_iso8601(const std::chrono::system_clock::time_point &tp) {
    using namespace std::chrono;
    auto ttime = system_clock::to_time_t(tp);
    std::tm tm;
#ifdef HAVE_GMTIME_S
    gmtime_s(&tm, &ttime);
#else
    gmtime_r(&ttime, &tm);
#endif
    auto micros = duration_cast<microseconds>(tp.time_since_epoch()).count() % 1000000;

    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    ss << "." << std::setw(6) << std::setfill('0') << micros << "Z";
    return ss.str();
}

} // namespace utils

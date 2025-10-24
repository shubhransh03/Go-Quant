#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <string>
#include <chrono>

namespace utils {
std::string to_iso8601(const std::chrono::system_clock::time_point &tp);
}

#endif // TIME_UTILS_H

#include "utils/logging.h"
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>

namespace utils {

static std::ofstream log_file;

void init_logging(const std::string& filename) {
    log_file.open(filename, std::ios::app);
    if (!log_file.is_open()) {
        std::cerr << "Failed to open log file: " << filename << std::endl;
    }
}

void log_info(const std::string& message) {
    if (log_file.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        log_file << "[INFO] " << std::ctime(&now_c) << ": " << message << std::endl;
    }
}

void log_error(const std::string& message) {
    if (log_file.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        log_file << "[ERROR] " << std::ctime(&now_c) << ": " << message << std::endl;
    }
}

void close_logging() {
    if (log_file.is_open()) {
        log_file.close();
    }
}

} // namespace utils
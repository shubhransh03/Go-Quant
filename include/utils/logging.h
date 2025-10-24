#ifndef LOGGING_H
#define LOGGING_H

#include <string>

namespace utils {

// Initialize logging to a file (append). Call once at startup.
void init_logging(const std::string &filename);

// Log at INFO level
void log_info(const std::string &message);

// Log at ERROR level
void log_error(const std::string &message);

// Close logging resources (call at shutdown)
void close_logging();

} // namespace utils

#endif // LOGGING_H
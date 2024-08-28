
#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <windows.h>

// Log levels
enum LogLevel {
    DEBUG,
    INFO,
    WARN,
    FAIL
};

// Convert log level to string
inline const char* LogLevelToString(LogLevel level) {
    switch (level) {
    case DEBUG: return "DEBUG";
    case INFO: return "INFO";
    case WARN: return "WARN";
    case FAIL: return "FAIL";
    default: return "UNKNOWN";
    }
}

// Base logger class
class Logger {
public:
    Logger(LogLevel level) : logLevel(level) {}

    void Log(LogLevel level, const std::string& message) {
        if (level >= logLevel) {
            std::stringstream logStream;
            logStream << LogLevelToString(level) << ": " << message;

            // Log to std::cout and std::cerr based on log level
            if (level == FAIL || level == WARN) {
                std::cerr << logStream.str() << std::endl;
            }
            else {
                std::cout << logStream.str() << std::endl;
            }

            // Log to debug output
            OutputDebugStringA(logStream.str().c_str());
        }
    }

private:
    LogLevel logLevel;
};

// Singleton logger instance
Logger& GetLogger() {
    static Logger logger(DEBUG); // Set the default log level here
    return logger;
}

// Macros for logging
#define LOG_DEBUG(format, ...)  Log(DEBUG, format "\n", __VA_ARGS__)
#define LOG_INFO(format, ...)   Log(INFO, format "\n", __VA_ARGS__)
#define LOG_WARN(format, ...)   Log(WARN, format "\n", __VA_ARGS__)
#define LOG_FAIL(format, ...)   Log(FAIL, format "\n", __VA_ARGS__)

inline void Log(LogLevel level, const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    GetLogger().Log(level, buffer);
}


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

    void Log( LogLevel level, const std::string& message );

private:
    LogLevel logLevel;
};

void Log( LogLevel level, const char* format, ... );

// Macros for logging
#define LOG_DEBUG(format, ...)  Log(DEBUG, format "\n", __VA_ARGS__)
#define LOG_INFO(format, ...)   Log(INFO,  format "\n", __VA_ARGS__)
#define LOG_WARN(format, ...)   Log(WARN,  format "\n", __VA_ARGS__)
#define LOG_FAIL(format, ...)   Log(FAIL,  format "\n", __VA_ARGS__)

// Log an error and throw a runtime_error
#define FATAL(format, ...) \
    do {                                            \
        LOG_FAIL( format, __VA_ARGS__ );            \
        throw std::runtime_error("");               \
    } while (false);                                \


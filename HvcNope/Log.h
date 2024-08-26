#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <mutex>

class Logger {
public:

    static enum class Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };

    Logger() : m_Level(Level::INFO) {}

    Logger(Level Level) : m_Level(Level) {}

    void setLevel(Level level) {
        m_Level = level;
    }

    template<typename... Args>
    void log(Level Level, const std::string& Format, Args&&... Arguments) {
        if (Level >= m_Level) {
            std::lock_guard<std::mutex> lock(m_Lock);
            std::ostream& out = (Level == Level::ERROR) ? std::cerr : std::cout;
            out << "[" << getLevelString(Level) << "] "
                << std::vformat(Format, std::make_format_args(std::forward<Args>(Arguments)...))
                << std::endl;
        }
    }

private:
    Level m_Level;
    std::mutex m_Lock;

    static std::string getLevelString(Level level) {
        switch (level) {
        case Level::DEBUG: return "DEBUG";
        case Level::INFO: return "INFO";
        case Level::WARNING: return "WARNING";
        case Level::ERROR: return "ERROR";
        }

        // unreachable
        __assume(0);
    }
};

extern Logger g_Logger;

// Macros for logging
#define LOG_DEBUG(message, ...)      g_Logger.log(Logger::Level::DEBUG, message, __VA_ARGS__)
#define LOG_INFO(message, ...)       g_Logger.log(Logger::Level::INFO, message, __VA_ARGS__)
#define LOG_WARNING(message, ...)    g_Logger.log(Logger::Level::WARNING, message, __VA_ARGS__)
#define LOG_ERROR(message, ...)      g_Logger.log(Logger::Level::ERROR, message, __VA_ARGS__)

#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <mutex>
#include <format>

class Logger {
public:
    enum LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };

    Logger() : m_Level(LogLevel::INFO) {}

    Logger(LogLevel Level) : m_Level(Level) {}

    void setLevel(LogLevel level) {
        m_Level = level;
    }

    template<typename... Args>
    void log(LogLevel Level, const std::string& Format, Args&&... Arguments) {
        if (Level >= m_Level) {
            std::lock_guard<std::mutex> lock(m_Lock);
            std::ostream& out = (Level == Level::ERROR) ? std::cerr : std::cout;
            out << "[" << getLevelString(Level) << "] "
                << std::vformat(Format, std::make_format_args(std::forward<Args>(Arguments)...))
                << std::endl;
        }
    }

private:
    LogLevel m_Level;
    std::mutex m_Lock;

    static std::string getLevelString(LogLevel level) {
        switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        }

        return "wtf";
    }
};

extern Logger g_Logger;

// Macros for logging
#define LOG_DEBUG(message, ...)      g_Logger.log(Logger::LogLevel::DEBUG, message, __VA_ARGS__);
#define LOG_INFO(message, ...)       g_Logger.log(Logger::LogLevel::INFO, message, __VA_ARGS__);
#define LOG_WARNING(message, ...)    g_Logger.log(Logger::LogLevel::WARNING, message, __VA_ARGS__);
#define LOG_ERROR(message, ...)      g_Logger.log(Logger::LogLevel::ERROR, message, __VA_ARGS__);

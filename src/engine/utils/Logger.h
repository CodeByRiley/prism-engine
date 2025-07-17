#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <typeinfo>

// Windows-specific includes
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#undef ERROR

// Define LogLevel enum
enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    FATAL,
    ERROR
};




class Logger {
public:
    // Delete copy constructor and assignment operator
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Get the singleton instance
    static Logger& Get() {
        static Logger instance;
        return instance;
    }

    // Initialize the logger
    static void Initialize(const std::string& logFile = "") {
        auto& logger = Get();
        if (!logFile.empty()) {
            logger.m_LogFile.open(logFile, std::ios::out | std::ios::app);
        }
    }

    // Log methods with class name
    template<typename T>
    static void Trace(const std::string& message, const T* instance = nullptr) {
        Log(LogLevel::TRACE, message, typeid(*instance).name());
    }

    template<typename T>
    static void Debug(const std::string& message, const T* instance = nullptr) {
        Log(LogLevel::DEBUG, message, typeid(*instance).name());
    }

    static void Info(const std::string& message)  {
        Log(LogLevel::INFO, message, "");
    }

    template<typename T>
    static void Warn(const std::string& message, const T* instance = nullptr)  {
        Log(LogLevel::WARN, message, instance ? typeid(*instance).name() : "");
    }

    template<typename T>
    static void Error(const std::string& message, const T* instance = nullptr) {
        Log(LogLevel::ERROR, message, instance ? typeid(*instance).name() : "");
    }

    template<typename T>
    static void Fatal(const std::string& message, const T* instance = nullptr) {
        Log(LogLevel::FATAL, message, instance ? typeid(*instance).name() : "");
    }

    // Set minimum log level
    static void SetLogLevel(LogLevel level) {
        Get().m_CurrentLevel = level;
    }

private:
    Logger() = default;
    ~Logger() {
        if (m_LogFile.is_open()) {
            m_LogFile.close();
        }
    }

    static std::string GetCurrentTime() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    static const char* LevelToString(LogLevel level) {
        switch (level) {
            case LogLevel::TRACE: return "TRACE";
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO ";
            case LogLevel::WARN:  return "WARN ";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FATAL: return "FATAL";
            default:              return "UNKNOWN";
        }
    }

    static std::string ExtractClassName(const std::string& prettyFunction) {
        // Format: "void ClassName::MethodName(args)" or "ReturnType ClassName::MethodName(args)"
        size_t colons = prettyFunction.rfind("::");
        if (colons == std::string::npos) return "";

        size_t begin = prettyFunction.rfind(" ", colons);
        if (begin == std::string::npos) begin = 0;
        else ++begin;

        return prettyFunction.substr(begin, colons - begin);
    }

    static void Log(LogLevel level, const std::string& message, const std::string& className = "") {
        auto& logger = Get();
        if (level < logger.m_CurrentLevel) return;

        std::stringstream ss;
        ss << "[" << GetCurrentTime() << "]"
           << "[" << LevelToString(level) << "]";

        if (!className.empty()) {
            ss << "[" << className << "] ";
        } else {
            ss << " ";
        }

        ss << message << std::endl;

        std::lock_guard<std::mutex> lock(logger.m_Mutex);
        std::cout << ss.str();
        if (logger.m_LogFile.is_open()) {
            logger.m_LogFile << ss.str();
            logger.m_LogFile.flush();
        }
    }

private:
    std::ofstream m_LogFile;
    std::mutex m_Mutex;
    LogLevel m_CurrentLevel = LogLevel::INFO;
};

// Helper macros for easier logging
#define LOG_TRACE(msg) Logger::Trace(msg, this)
#define LOG_DEBUG(msg) Logger::Debug(msg, this)
#define LOG_INFO(msg)  Logger::Info(msg)
#define LOG_WARN(msg)  Logger::Warn(msg, this)
#define LOG_ERROR(msg) Logger::Error(msg, this)
#define LOG_FATAL(msg) Logger::Fatal(msg, this)
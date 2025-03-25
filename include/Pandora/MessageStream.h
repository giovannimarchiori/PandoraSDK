#ifndef PANDORA_MESSAGE_STREAM_H
#define PANDORA_MESSAGE_STREAM_H_1

#include <iostream>
#include <iomanip>
#include <string>
#include <mutex>

// for demangling - GCC/Clang
#include <cxxabi.h>

namespace pandora {
    
class MessageStream {
public:
    // Define log levels
    enum class Level { VERBOSE, DEBUG, INFO, WARNING, ERROR };

    // Constructor (default log level is INFO)
    MessageStream();

    // Set log level dynamically
    void setLogLevel(Level level);

    // Log a message with function name and severity
    void log(Level level, const std::string& objName, const std::string& className, const std::string& function, const std::string& message) const;

private:
    mutable std::mutex mutex;  // Protects concurrent access
    Level logLevel;            // Default log level is INFO

    // Convert enum to string for printing
    static std::string levelToString(Level level);

    // demangle a string
    static std::string demangle(const char* mangled);
};

inline MessageStream::MessageStream() :
    logLevel(Level::INFO) {
}

// Set log level dynamically
inline void MessageStream::setLogLevel(Level level) {
    std::lock_guard<std::mutex> lock(mutex);
    logLevel = level;
}

// Log a message with function name and severity
inline void MessageStream::log(Level level, const std::string& objName, const std::string& className, const std::string& function, const std::string& message) const {
    std::lock_guard<std::mutex> lock(mutex);  // Ensure thread safety

    // Only print if message level is >= current log level
    if (level >= logLevel) {
        std::cout
            << std::left << std::setw(20)
            << objName.substr(0, 20) << "  "
            << std::left << std::setw(30)
            << demangle(className.c_str()).substr(0, 30) << "  "
            << std::left << std::setw(20)
            << function.substr(0, 20) << "  "
            << std::left << std::setw(10)
            << levelToString(level) << "  "
            << message << std::endl;
    }
}

// Convert enum to string for printing
inline std::string MessageStream::levelToString(Level level) {
    switch (level) {
        case Level::VERBOSE: return "VERBOSE"; 
        case Level::DEBUG:   return "DEBUG";
        case Level::INFO:    return "INFO";
        case Level::WARNING: return "WARNING";
        case Level::ERROR:   return "ERROR";
        default:             return "UNKNOWN";
    }
}

inline std::string MessageStream::demangle(const char* mangled) {
    int status;
    char* demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
    std::string result = (status == 0) ? demangled : mangled;
    // remove namespace info
    size_t pos = result.rfind("::");  // Find last occurrence of "::"
    if (pos != std::string::npos) {
        result = result.substr(pos + 2);  // Keep only what comes after "::"
    }
    free(demangled);
    return result;
}

// Helper macros for easy logging
//#define LOG_VERBOSE(msg)   log(MessageStream::Level::VERBOSE, typeid(*this).name(), __FUNCTION__, msg)
//#define LOG_DEBUG(msg)   log(MessageStream::Level::DEBUG, typeid(*this).name(), __FUNCTION__, msg)
//#define LOG_INFO(msg)    log(MessageStream::Level::INFO, typeid(*this).name(), __FUNCTION__, msg)
//#define LOG_WARNING(msg) log(MessageStream::Level::WARNING, typeid(*this).name(), __FUNCTION__, msg)
//#define LOG_ERROR(msg)   log(MessageStream::Level::ERROR, typeid(*this).name(), __FUNCTION__, msg)

} // end namespace Pandora
#endif // MESSAGE_STREAM_H


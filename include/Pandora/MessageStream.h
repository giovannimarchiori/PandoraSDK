#ifndef PANDORA_MESSAGE_STREAM_H
#define PANDORA_MESSAGE_STREAM_H 1

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
    void setLogLevel(const std::string& level);

    // Get log level
    std::string getLogLevel() const;

    // Log a message with function name and severity
    // old - pass message to log function
    // void log(Level level, const std::string& objName, const std::string& className, const std::string& function, const std::string& message) const;
    // new - use log(..)  << message << std::endl; so that one can concatenate various pieces of info
    std::ostream& log(Level level, const std::string& objName, const std::string& className, const std::string& function) const;

    // getters/setters for default output level
    static void setDefaultLogLevel(Level level) { defaultLogLevel = level; }
    static Level getDefaultLogLevel() { return defaultLogLevel; }
    static void setDefaultLogLevel(const std::string& level) { defaultLogLevel = stringToLevel(level); }

private:
    mutable std::mutex mutex;     // Protects concurrent access
    Level logLevel;               // Current log level
    static Level defaultLogLevel; // Default log level (usually INFO)
    mutable std::ostream nullStream{nullptr};

    // Convert enum to string for printing
    static std::string levelToString(Level level);

    // Convert string to enum for parsing xml files
    static Level stringToLevel(const std::string& s);

    // demangle a string
    static std::string demangle(const char* mangled);
};

inline MessageStream::MessageStream() :
    logLevel(defaultLogLevel) {
}

// Set log level dynamically
inline void MessageStream::setLogLevel(Level level) {
    std::lock_guard<std::mutex> lock(mutex);
    logLevel = level;
}

inline void MessageStream::setLogLevel(const std::string& level) {
    std::lock_guard<std::mutex> lock(mutex);
    logLevel = stringToLevel(level);
}

// Get log level
inline std::string MessageStream::getLogLevel() const {
    return levelToString(logLevel);
}
        
// Log a message with function name and severity
/*
inline void MessageStream::log(Level level, const std::string& objName, const std::string& className, const std::string& function, const std::string& message) const {
    std::lock_guard<std::mutex> lock(mutex);  // Ensure thread safety

    (void) function; // silence "unused parameter" warning

    // Only print if message level is >= current log level
    if (level >= logLevel) {
        std::cout
            << std::left << std::setw(10)
            << objName.substr(0, 10) << "  "
            << std::left << std::setw(30)
            << demangle(className.c_str()).substr(0, 30) << "  "
            // << std::left << std::setw(20)
            // << function.substr(0, 20) << "  "
            << std::left << std::setw(7)
            << levelToString(level) << "  "
            << message << std::endl;
    }
}
*/

inline std::ostream& MessageStream::log(Level level, const std::string& objName, const std::string& className, const std::string& function) const {
    std::lock_guard<std::mutex> lock(mutex);  // Ensure thread safety

    (void) function; // silence "unused parameter" warning

    // Only print if message level is >= current log level
    if (level >= logLevel) {
        return std::cout
            << std::left << std::setw(10)
            << objName.substr(0, 10) << "  "
            << std::left << std::setw(30)
            << demangle(className.c_str()).substr(0, 30) << "  "
            // << std::left << std::setw(20)
            // << function.substr(0, 20) << "  "
            << std::left << std::setw(7)
            << levelToString(level) << "  ";
    }
    else {
        return nullStream;
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

inline MessageStream::Level MessageStream::stringToLevel(const std::string& s) {
    if (s == "VERBOSE")
        return Level::VERBOSE;
    else if (s == "DEBUG")
        return Level::DEBUG;
    else if (s == "INFO")
        return Level::INFO;
    else if (s == "WARNING")
        return Level::WARNING;
    else if (s == "ERROR")
        return Level::ERROR;
    else {
        std::cout << "Unknown output level " << s << ", returning default (INFO)" << std::endl;
        return Level::INFO;
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

} // end namespace Pandora
#endif // MESSAGE_STREAM_H


#pragma once

#include "pxpoint_correlation.h"
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <string>
#include <memory>
#include <unordered_map>

namespace pxpoint {

/**
 * PxPoint-specific logger that integrates with the correlation system
 * and writes to /tmp/pxpoint-logs/
 */
class PxPointLogger {
public:
    static PxPointLogger& getInstance();
    
    // Initialize the logger for a specific process type
    void initialize(const std::string& process_type, 
                   spdlog::level::level_enum log_level = spdlog::level::info);
    
    // Log with context and correlation
    void logWithContext(spdlog::level::level_enum level,
                       const std::string& component,
                       const std::string& message,
                       const std::unordered_map<std::string, std::string>& context = {},
                       const std::unordered_map<std::string, double>& performance = {});
    
    // Shutdown async logging
    void shutdown();
    
    // Log process milestone events
    void logProcessStart(const std::string& process_type, 
                        const std::unordered_map<std::string, std::string>& config = {});
    void logProcessEnd(const std::string& process_type, bool success = true,
                      const std::unordered_map<std::string, double>& metrics = {});
    
    // Log activity events
    void logActivityStart(const std::string& activity_name,
                         const std::unordered_map<std::string, std::string>& context = {});
    void logActivityEnd(const std::string& activity_name, bool success = true,
                       const std::unordered_map<std::string, double>& metrics = {});
    
    // Log error with full context
    void logError(const std::string& component, const std::string& message,
                 const std::string& exception = "",
                 const std::unordered_map<std::string, std::string>& context = {});

private:
    PxPointLogger() = default;
    
    bool initialized_ = false;
    std::string process_type_;
    std::shared_ptr<spdlog::async_logger> logger_;
    
    std::string formatStructuredMessage(const std::string& message,
                                      const std::unordered_map<std::string, std::string>& context,
                                      const std::unordered_map<std::string, double>& performance);
    
    std::string getCurrentTimestamp();
};

// Convenience macros for PxPoint logging
#define PXPOINT_LOG_INFO(component, message, ...) \
    pxpoint::PxPointLogger::getInstance().logWithContext(spdlog::level::info, component, message, ##__VA_ARGS__)

#define PXPOINT_LOG_WARN(component, message, ...) \
    pxpoint::PxPointLogger::getInstance().logWithContext(spdlog::level::warn, component, message, ##__VA_ARGS__)

#define PXPOINT_LOG_ERROR(component, message, ...) \
    pxpoint::PxPointLogger::getInstance().logWithContext(spdlog::level::err, component, message, ##__VA_ARGS__)

#define PXPOINT_LOG_DEBUG(component, message, ...) \
    pxpoint::PxPointLogger::getInstance().logWithContext(spdlog::level::debug, component, message, ##__VA_ARGS__)

// Activity scope logging
#define PXPOINT_ACTIVITY_SCOPE(activity_name) \
    pxpoint::ActivityCorrelationScope _activity_scope(activity_name); \
    pxpoint::PxPointLogger::getInstance().logActivityStart(activity_name)

} // namespace pxpoint
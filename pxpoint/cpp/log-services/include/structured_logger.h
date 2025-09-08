#pragma once

#include "correlation_manager.h"
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <vector>

namespace logservices {

/**
 * Sink configuration for structured logging
 */
struct SinkConfig {
    enum class Type {
        Console,
        File,
        RotatingFile,
        DailyFile
    };
    
    Type type;
    std::string name;
    spdlog::level::level_enum level = spdlog::level::info;
    std::string pattern;
    
    // File-specific options
    std::string file_path;
    size_t max_file_size = 1024 * 1024 * 10; // 10MB
    size_t max_files = 5;
    int rotation_hour = 0;
    int rotation_minute = 0;
    
    // Console-specific options
    bool color_mode = true;
};

/**
 * Logger configuration structure
 */
struct LoggerConfig {
    std::string name = "app";
    spdlog::level::level_enum level = spdlog::level::info;
    std::string default_pattern = "%Y-%m-%dT%H:%M:%S.%fZ [%l] [%n] %v";
    std::string json_pattern = R"({"timestamp":"%Y-%m-%dT%H:%M:%S.%fZ","level":"%^%l%$","logger":"%n","message":"%v"})";
    
    // Async logging configuration
    bool async_logging = true;
    size_t async_queue_size = 8192;
    size_t async_thread_count = 1;
    spdlog::async_overflow_policy async_overflow_policy = spdlog::async_overflow_policy::block;
    
    // Output directory
    std::string log_directory = "/tmp/pxpoint-logs";
    
    // Sinks configuration
    std::vector<SinkConfig> sinks;
    
    // Automatic correlation integration
    bool auto_add_correlation = true;
    
    // Custom formatters
    std::function<std::string(const std::string&, const std::unordered_map<std::string, std::string>&)> custom_formatter;
    
    // Error handling
    bool flush_on_error = true;
    std::chrono::seconds flush_interval{5};
};

/**
 * Generic structured logger with YAML configuration support
 */
class StructuredLogger {
public:
    static StructuredLogger& getInstance();
    
    // Configuration
    void configure(const LoggerConfig& config);
    void loadConfigFromYaml(const std::string& yaml_file_path);
    
    // Initialize with process type
    void initialize(const std::string& process_type, 
                   spdlog::level::level_enum log_level = spdlog::level::info);
    
    // Core logging with context
    void log(spdlog::level::level_enum level,
             const std::string& message,
             const std::unordered_map<std::string, std::string>& context = {},
             const std::unordered_map<std::string, double>& metrics = {});
    
    void log(spdlog::level::level_enum level,
             const std::string& component,
             const std::string& message,
             const std::unordered_map<std::string, std::string>& context = {},
             const std::unordered_map<std::string, double>& metrics = {});
    
    // Convenience methods
    void debug(const std::string& message, const std::unordered_map<std::string, std::string>& context = {});
    void info(const std::string& message, const std::unordered_map<std::string, std::string>& context = {});
    void warn(const std::string& message, const std::unordered_map<std::string, std::string>& context = {});
    void error(const std::string& message, const std::unordered_map<std::string, std::string>& context = {});
    void critical(const std::string& message, const std::unordered_map<std::string, std::string>& context = {});
    
    void debug(const std::string& component, const std::string& message, const std::unordered_map<std::string, std::string>& context = {});
    void info(const std::string& component, const std::string& message, const std::unordered_map<std::string, std::string>& context = {});
    void warn(const std::string& component, const std::string& message, const std::unordered_map<std::string, std::string>& context = {});
    void error(const std::string& component, const std::string& message, const std::unordered_map<std::string, std::string>& context = {});
    void critical(const std::string& component, const std::string& message, const std::unordered_map<std::string, std::string>& context = {});
    
    // Event logging
    void logEvent(const std::string& event_type,
                  const std::string& description,
                  const std::unordered_map<std::string, std::string>& context = {},
                  const std::unordered_map<std::string, double>& metrics = {});
    
    // Process lifecycle logging
    void logProcessStart(const std::string& process_type, 
                        const std::unordered_map<std::string, std::string>& config = {});
    void logProcessEnd(const std::string& process_type, bool success = true,
                      const std::unordered_map<std::string, double>& metrics = {});
    
    // Activity lifecycle logging
    void logActivityStart(const std::string& activity_name,
                         const std::unordered_map<std::string, std::string>& context = {});
    void logActivityEnd(const std::string& activity_name, bool success = true,
                       const std::unordered_map<std::string, double>& metrics = {});
    
    // Performance logging
    void logPerformance(const std::string& operation,
                       double duration_ms,
                       const std::unordered_map<std::string, std::string>& context = {},
                       const std::unordered_map<std::string, double>& metrics = {});
    
    // Error logging with exception support
    void logError(const std::string& component, 
                 const std::string& message,
                 const std::string& exception = "",
                 const std::unordered_map<std::string, std::string>& context = {});
    
    // Flush and shutdown
    void flush();
    void shutdown();
    
    // Logger management
    std::shared_ptr<spdlog::logger> getLogger() const;
    void setLevel(spdlog::level::level_enum level);
    spdlog::level::level_enum getLevel() const;
    
private:
    StructuredLogger() = default;
    
    bool initialized_ = false;
    std::string process_type_;
    LoggerConfig config_;
    std::shared_ptr<spdlog::logger> logger_;
    
    // Internal methods
    void createSinks();
    std::shared_ptr<spdlog::sinks::sink> createSink(const SinkConfig& sink_config);
    std::string formatMessage(const std::string& message,
                             const std::unordered_map<std::string, std::string>& context,
                             const std::unordered_map<std::string, double>& metrics);
    std::string getCurrentTimestamp();
    void ensureDirectoryExists(const std::string& path);
};

// Performance measurement helper
class PerformanceTimer {
public:
    explicit PerformanceTimer(const std::string& operation_name,
                             const std::unordered_map<std::string, std::string>& context = {});
    ~PerformanceTimer();
    
    void addContext(const std::string& key, const std::string& value);
    void addMetric(const std::string& key, double value);
    void stop();
    
private:
    std::string operation_name_;
    std::unordered_map<std::string, std::string> context_;
    std::unordered_map<std::string, double> metrics_;
    std::chrono::high_resolution_clock::time_point start_time_;
    bool stopped_;
};

} // namespace logservices

// Suppress GNU extension warnings for variadic macros
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

// Convenience macros for structured logging
#define LOG_DEBUG(message, ...) \
    logservices::StructuredLogger::getInstance().debug(message, ##__VA_ARGS__)

#define LOG_INFO(message, ...) \
    logservices::StructuredLogger::getInstance().info(message, ##__VA_ARGS__)

#define LOG_WARN(message, ...) \
    logservices::StructuredLogger::getInstance().warn(message, ##__VA_ARGS__)

#define LOG_ERROR(message, ...) \
    logservices::StructuredLogger::getInstance().error(message, ##__VA_ARGS__)

#define LOG_CRITICAL(message, ...) \
    logservices::StructuredLogger::getInstance().critical(message, ##__VA_ARGS__)

// Component-specific logging macros
#define LOG_COMPONENT_DEBUG(component, message, ...) \
    logservices::StructuredLogger::getInstance().debug(component, message, ##__VA_ARGS__)

#define LOG_COMPONENT_INFO(component, message, ...) \
    logservices::StructuredLogger::getInstance().info(component, message, ##__VA_ARGS__)

#define LOG_COMPONENT_WARN(component, message, ...) \
    logservices::StructuredLogger::getInstance().warn(component, message, ##__VA_ARGS__)

#define LOG_COMPONENT_ERROR(component, message, ...) \
    logservices::StructuredLogger::getInstance().error(component, message, ##__VA_ARGS__)

#define LOG_COMPONENT_CRITICAL(component, message, ...) \
    logservices::StructuredLogger::getInstance().critical(component, message, ##__VA_ARGS__)

// Performance timing macros
#define LOG_PERFORMANCE_SCOPE(operation_name, ...) \
    logservices::PerformanceTimer _perf_timer(operation_name, ##__VA_ARGS__)

#define LOG_ACTIVITY_SCOPE(activity_name, ...) \
    logservices::ActivityScope _activity_scope(activity_name, ##__VA_ARGS__); \
    logservices::StructuredLogger::getInstance().logActivityStart(activity_name, ##__VA_ARGS__)

#pragma clang diagnostic pop
#pragma GCC diagnostic pop

#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/fmt/ostr.h>
#include <yaml-cpp/yaml.h>
#include <string>
#include <memory>
#include <unordered_map>

namespace gis {

class Logger {
public:
    static Logger& getInstance();
    
    // Initialize with YAML configuration file
    void initialize(const std::string& config_file = "plogger/config/logging.yaml");
    
    // Legacy initialization method (for backward compatibility)
    void initialize(const std::string& log_level, 
                   const std::string& log_file,
                   size_t max_file_size = 5 * 1024 * 1024, // 5MB
                   size_t max_files = 3);
    
    std::shared_ptr<spdlog::logger> getLogger(const std::string& name = "default");
    
    void setCorrelationId(const std::string& correlation_id);
    std::string getCorrelationId() const;
    
    void logWithContext(spdlog::level::level_enum level,
                       const std::string& logger_name,
                       const std::string& message,
                       const std::unordered_map<std::string, std::string>& context = {},
                       const std::unordered_map<std::string, double>& performance = {});

private:
    Logger() = default;
    bool initialized_ = false;
    std::string correlation_id_;
    std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> loggers_;
    YAML::Node config_;
    
    std::string formatStructuredMessage(const std::string& message,
                                      const std::unordered_map<std::string, std::string>& context,
                                      const std::unordered_map<std::string, double>& performance);
    
    void loadConfig(const std::string& config_file);
    spdlog::level::level_enum parseLogLevel(const std::string& level_str);
};

// Convenience macros for structured logging
#define LOG_WITH_CORRELATION(level, logger_name, message, ...) \
    do { \
        auto logger = gis::Logger::getInstance().getLogger(logger_name); \
        if (logger->should_log(level)) { \
            gis::Logger::getInstance().logWithContext(level, logger_name, message, ##__VA_ARGS__); \
        } \
    } while(0)

#define LOG_INFO(logger_name, message, ...) LOG_WITH_CORRELATION(spdlog::level::info, logger_name, message, ##__VA_ARGS__)
#define LOG_WARN(logger_name, message, ...) LOG_WITH_CORRELATION(spdlog::level::warn, logger_name, message, ##__VA_ARGS__)
#define LOG_ERROR(logger_name, message, ...) LOG_WITH_CORRELATION(spdlog::level::err, logger_name, message, ##__VA_ARGS__)
#define LOG_DEBUG(logger_name, message, ...) LOG_WITH_CORRELATION(spdlog::level::debug, logger_name, message, ##__VA_ARGS__)

} // namespace gis
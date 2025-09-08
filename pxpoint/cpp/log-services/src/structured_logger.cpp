#include "structured_logger.h"
#include <yaml-cpp/yaml.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <stdexcept>

namespace logservices {

StructuredLogger& StructuredLogger::getInstance() {
    static StructuredLogger instance;
    return instance;
}

void StructuredLogger::configure(const LoggerConfig& config) {
    config_ = config;
}

void StructuredLogger::loadConfigFromYaml(const std::string& yaml_file_path) {
    try {
        YAML::Node config = YAML::LoadFile(yaml_file_path);
        
        if (config["logging"]) {
            auto logging_config = config["logging"];
            
            if (logging_config["name"]) {
                config_.name = logging_config["name"].as<std::string>();
            }
            
            if (logging_config["level"]) {
                std::string level_str = logging_config["level"].as<std::string>();
                config_.level = spdlog::level::from_str(level_str);
            }
            
            if (logging_config["default_pattern"]) {
                config_.default_pattern = logging_config["default_pattern"].as<std::string>();
            }
            
            if (logging_config["json_pattern"]) {
                config_.json_pattern = logging_config["json_pattern"].as<std::string>();
            }
            
            if (logging_config["async_logging"]) {
                config_.async_logging = logging_config["async_logging"].as<bool>();
            }
            
            if (logging_config["async_queue_size"]) {
                config_.async_queue_size = logging_config["async_queue_size"].as<size_t>();
            }
            
            if (logging_config["async_thread_count"]) {
                config_.async_thread_count = logging_config["async_thread_count"].as<size_t>();
            }
            
            if (logging_config["log_directory"]) {
                config_.log_directory = logging_config["log_directory"].as<std::string>();
            }
            
            if (logging_config["auto_add_correlation"]) {
                config_.auto_add_correlation = logging_config["auto_add_correlation"].as<bool>();
            }
            
            if (logging_config["flush_on_error"]) {
                config_.flush_on_error = logging_config["flush_on_error"].as<bool>();
            }
            
            if (logging_config["flush_interval"]) {
                int interval = logging_config["flush_interval"].as<int>();
                config_.flush_interval = std::chrono::seconds(interval);
            }
            
            // Parse sinks configuration
            if (logging_config["sinks"]) {
                config_.sinks.clear();
                for (const auto& sink_node : logging_config["sinks"]) {
                    SinkConfig sink_config;
                    
                    if (sink_node["type"]) {
                        std::string type_str = sink_node["type"].as<std::string>();
                        if (type_str == "console") {
                            sink_config.type = SinkConfig::Type::Console;
                        } else if (type_str == "file") {
                            sink_config.type = SinkConfig::Type::File;
                        } else if (type_str == "rotating_file") {
                            sink_config.type = SinkConfig::Type::RotatingFile;
                        } else if (type_str == "daily_file") {
                            sink_config.type = SinkConfig::Type::DailyFile;
                        }
                    }
                    
                    if (sink_node["name"]) {
                        sink_config.name = sink_node["name"].as<std::string>();
                    }
                    
                    if (sink_node["level"]) {
                        std::string level_str = sink_node["level"].as<std::string>();
                        sink_config.level = spdlog::level::from_str(level_str);
                    }
                    
                    if (sink_node["pattern"]) {
                        sink_config.pattern = sink_node["pattern"].as<std::string>();
                    }
                    
                    if (sink_node["file_path"]) {
                        sink_config.file_path = sink_node["file_path"].as<std::string>();
                    }
                    
                    if (sink_node["max_file_size"]) {
                        sink_config.max_file_size = sink_node["max_file_size"].as<size_t>();
                    }
                    
                    if (sink_node["max_files"]) {
                        sink_config.max_files = sink_node["max_files"].as<size_t>();
                    }
                    
                    if (sink_node["rotation_hour"]) {
                        sink_config.rotation_hour = sink_node["rotation_hour"].as<int>();
                    }
                    
                    if (sink_node["rotation_minute"]) {
                        sink_config.rotation_minute = sink_node["rotation_minute"].as<int>();
                    }
                    
                    if (sink_node["color_mode"]) {
                        sink_config.color_mode = sink_node["color_mode"].as<bool>();
                    }
                    
                    config_.sinks.push_back(sink_config);
                }
            }
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to load logging config from YAML: " + std::string(e.what()));
    }
}

void StructuredLogger::initialize(const std::string& process_type, spdlog::level::level_enum log_level) {
    if (initialized_) {
        return;
    }
    
    process_type_ = process_type;
    
    // Set default level if not configured
    if (config_.level == spdlog::level::off) {
        config_.level = log_level;
    }
    
    // If no sinks configured, create default ones
    if (config_.sinks.empty()) {
        // Console sink
        SinkConfig console_sink;
        console_sink.type = SinkConfig::Type::Console;
        console_sink.name = "console";
        console_sink.level = spdlog::level::info;
        console_sink.pattern = config_.default_pattern;
        config_.sinks.push_back(console_sink);
        
        // Daily file sink
        SinkConfig file_sink;
        file_sink.type = SinkConfig::Type::DailyFile;
        file_sink.name = "daily_file";
        file_sink.level = spdlog::level::debug;
        file_sink.pattern = config_.json_pattern;
        file_sink.file_path = config_.log_directory + "/" + process_type + "-{}.log";
        config_.sinks.push_back(file_sink);
    }
    
    try {
        // Ensure log directory exists
        ensureDirectoryExists(config_.log_directory);
        
        // Initialize async thread pool if async logging is enabled
        if (config_.async_logging) {
            try {
                spdlog::init_thread_pool(config_.async_queue_size, config_.async_thread_count);
            } catch (const std::exception&) {
                // Thread pool already initialized
            }
        }
        
        // Create sinks
        createSinks();
        
        initialized_ = true;
        
        // Log initialization
        info("Logger", "Structured logger initialized", 
             {{"process_type", process_type}, {"log_directory", config_.log_directory}});
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to initialize structured logger: " + std::string(e.what()));
    }
}

void StructuredLogger::createSinks() {
    std::vector<spdlog::sink_ptr> sinks;
    
    for (const auto& sink_config : config_.sinks) {
        auto sink = createSink(sink_config);
        if (sink) {
            sinks.push_back(sink);
        }
    }
    
    if (sinks.empty()) {
        throw std::runtime_error("No valid sinks configured for structured logger");
    }
    
    // Create logger
    if (config_.async_logging) {
        logger_ = std::make_shared<spdlog::async_logger>(
            config_.name,
            sinks.begin(),
            sinks.end(),
            spdlog::thread_pool(),
            config_.async_overflow_policy
        );
    } else {
        logger_ = std::make_shared<spdlog::logger>(
            config_.name,
            sinks.begin(),
            sinks.end()
        );
    }
    
    logger_->set_level(config_.level);
    spdlog::register_logger(logger_);
    
    // Set up automatic flushing
    if (config_.flush_on_error) {
        logger_->flush_on(spdlog::level::err);
    }
    
    spdlog::flush_every(config_.flush_interval);
}

std::shared_ptr<spdlog::sinks::sink> StructuredLogger::createSink(const SinkConfig& sink_config) {
    std::shared_ptr<spdlog::sinks::sink> sink;
    
    switch (sink_config.type) {
        case SinkConfig::Type::Console: {
            // Always use color sink - simpler and more widely supported
            sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            break;
        }
        
        case SinkConfig::Type::File: {
            std::string file_path = sink_config.file_path;
            if (file_path.find("{}") != std::string::npos) {
                // Replace {} with process type
                size_t pos = file_path.find("{}");
                file_path.replace(pos, 2, process_type_);
            }
            sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(file_path, true);
            break;
        }
        
        case SinkConfig::Type::RotatingFile: {
            std::string file_path = sink_config.file_path;
            if (file_path.find("{}") != std::string::npos) {
                size_t pos = file_path.find("{}");
                file_path.replace(pos, 2, process_type_);
            }
            sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                file_path, sink_config.max_file_size, sink_config.max_files);
            break;
        }
        
        case SinkConfig::Type::DailyFile: {
            std::string file_path = sink_config.file_path;
            if (file_path.find("{}") != std::string::npos) {
                size_t pos = file_path.find("{}");
                file_path.replace(pos, 2, process_type_);
            }
            sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
                file_path, sink_config.rotation_hour, sink_config.rotation_minute);
            break;
        }
    }
    
    if (sink) {
        sink->set_level(sink_config.level);
        if (!sink_config.pattern.empty()) {
            sink->set_pattern(sink_config.pattern);
        }
    }
    
    return sink;
}

std::string StructuredLogger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return ss.str();
}

std::string StructuredLogger::formatMessage(
    const std::string& message,
    const std::unordered_map<std::string, std::string>& context,
    const std::unordered_map<std::string, double>& metrics) {
    
    if (config_.custom_formatter) {
        return config_.custom_formatter(message, context);
    }
    
    std::stringstream ss;
    ss << message;
    
    // Add correlation information if enabled
    if (config_.auto_add_correlation) {
        auto& correlation_manager = CorrelationManager::getInstance();
        auto correlation_context = correlation_manager.getCorrelationContext();
        
        if (!correlation_context.empty()) {
            ss << " | ";
            for (const auto& [key, value] : correlation_context) {
                ss << key << ":" << value << " ";
            }
        }
    }
    
    // Add context fields
    if (!context.empty() || !metrics.empty()) {
        ss << " | ";
        
        for (const auto& [key, value] : context) {
            ss << key << ":" << value << " ";
        }
        
        // Add performance fields
        for (const auto& [key, value] : metrics) {
            ss << key << ":" << std::fixed << std::setprecision(2) << value << " ";
        }
    }
    
    return ss.str();
}

void StructuredLogger::ensureDirectoryExists(const std::string& path) {
    try {
        std::filesystem::create_directories(path);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to create log directory: " + path + " - " + e.what());
    }
}

// Core logging methods
void StructuredLogger::log(spdlog::level::level_enum level,
                          const std::string& message,
                          const std::unordered_map<std::string, std::string>& context,
                          const std::unordered_map<std::string, double>& metrics) {
    if (!initialized_) {
        throw std::runtime_error("Structured logger not initialized");
    }
    
    std::string formatted_message = formatMessage(message, context, metrics);
    logger_->log(level, formatted_message);
}

void StructuredLogger::log(spdlog::level::level_enum level,
                          const std::string& component,
                          const std::string& message,
                          const std::unordered_map<std::string, std::string>& context,
                          const std::unordered_map<std::string, double>& metrics) {
    auto extended_context = context;
    extended_context["component"] = component;
    log(level, message, extended_context, metrics);
}

// Convenience methods
void StructuredLogger::debug(const std::string& message, const std::unordered_map<std::string, std::string>& context) {
    log(spdlog::level::debug, message, context);
}

void StructuredLogger::info(const std::string& message, const std::unordered_map<std::string, std::string>& context) {
    log(spdlog::level::info, message, context);
}

void StructuredLogger::warn(const std::string& message, const std::unordered_map<std::string, std::string>& context) {
    log(spdlog::level::warn, message, context);
}

void StructuredLogger::error(const std::string& message, const std::unordered_map<std::string, std::string>& context) {
    log(spdlog::level::err, message, context);
}

void StructuredLogger::critical(const std::string& message, const std::unordered_map<std::string, std::string>& context) {
    log(spdlog::level::critical, message, context);
}

void StructuredLogger::debug(const std::string& component, const std::string& message, const std::unordered_map<std::string, std::string>& context) {
    log(spdlog::level::debug, component, message, context);
}

void StructuredLogger::info(const std::string& component, const std::string& message, const std::unordered_map<std::string, std::string>& context) {
    log(spdlog::level::info, component, message, context);
}

void StructuredLogger::warn(const std::string& component, const std::string& message, const std::unordered_map<std::string, std::string>& context) {
    log(spdlog::level::warn, component, message, context);
}

void StructuredLogger::error(const std::string& component, const std::string& message, const std::unordered_map<std::string, std::string>& context) {
    log(spdlog::level::err, component, message, context);
}

void StructuredLogger::critical(const std::string& component, const std::string& message, const std::unordered_map<std::string, std::string>& context) {
    log(spdlog::level::critical, component, message, context);
}

// Event logging
void StructuredLogger::logEvent(const std::string& event_type,
                               const std::string& description,
                               const std::unordered_map<std::string, std::string>& context,
                               const std::unordered_map<std::string, double>& metrics) {
    auto event_context = context;
    event_context["event_type"] = event_type;
    event_context["timestamp"] = getCurrentTimestamp();
    
    log(spdlog::level::info, description, event_context, metrics);
}

// Process lifecycle logging
void StructuredLogger::logProcessStart(const std::string& process_type,
                                      const std::unordered_map<std::string, std::string>& config) {
    auto context = config;
    context["event_type"] = "process_start";
    context["process_type"] = process_type;
    context["timestamp"] = getCurrentTimestamp();
    
    log(spdlog::level::info, "Process started: " + process_type, context);
}

void StructuredLogger::logProcessEnd(const std::string& process_type, bool success,
                                    const std::unordered_map<std::string, double>& metrics) {
    std::unordered_map<std::string, std::string> context;
    context["event_type"] = "process_end";
    context["process_type"] = process_type;
    context["success"] = success ? "true" : "false";
    context["timestamp"] = getCurrentTimestamp();
    
    auto log_level = success ? spdlog::level::info : spdlog::level::err;
    std::string message = success ? "Process completed successfully: " : "Process failed: ";
    message += process_type;
    
    log(log_level, message, context, metrics);
}

// Activity lifecycle logging
void StructuredLogger::logActivityStart(const std::string& activity_name,
                                       const std::unordered_map<std::string, std::string>& context) {
    auto activity_context = context;
    activity_context["event_type"] = "activity_start";
    activity_context["activity_name"] = activity_name;
    activity_context["timestamp"] = getCurrentTimestamp();
    
    log(spdlog::level::debug, "Activity started: " + activity_name, activity_context);
}

void StructuredLogger::logActivityEnd(const std::string& activity_name, bool success,
                                     const std::unordered_map<std::string, double>& metrics) {
    std::unordered_map<std::string, std::string> context;
    context["event_type"] = "activity_end";
    context["activity_name"] = activity_name;
    context["success"] = success ? "true" : "false";
    context["timestamp"] = getCurrentTimestamp();
    
    auto log_level = success ? spdlog::level::debug : spdlog::level::warn;
    std::string message = success ? "Activity completed: " : "Activity failed: ";
    message += activity_name;
    
    log(log_level, message, context, metrics);
}

// Performance logging
void StructuredLogger::logPerformance(const std::string& operation,
                                     double duration_ms,
                                     const std::unordered_map<std::string, std::string>& context,
                                     const std::unordered_map<std::string, double>& metrics) {
    auto perf_context = context;
    perf_context["event_type"] = "performance";
    perf_context["operation"] = operation;
    perf_context["timestamp"] = getCurrentTimestamp();
    
    auto perf_metrics = metrics;
    perf_metrics["duration_ms"] = duration_ms;
    
    log(spdlog::level::info, "Performance measurement: " + operation, perf_context, perf_metrics);
}

// Error logging with exception support
void StructuredLogger::logError(const std::string& component,
                               const std::string& message,
                               const std::string& exception,
                               const std::unordered_map<std::string, std::string>& context) {
    auto error_context = context;
    error_context["event_type"] = "error";
    error_context["component"] = component;
    error_context["timestamp"] = getCurrentTimestamp();
    if (!exception.empty()) {
        error_context["exception"] = exception;
    }
    
    std::string error_message = "ERROR: " + message;
    if (!exception.empty()) {
        error_message += " | Exception: " + exception;
    }
    
    log(spdlog::level::err, error_message, error_context);
}

// Management methods
void StructuredLogger::flush() {
    if (logger_) {
        logger_->flush();
    }
}

void StructuredLogger::shutdown() {
    if (logger_) {
        logger_->flush();
        spdlog::drop(logger_->name());
    }
    
    try {
        spdlog::shutdown();
    } catch (const std::exception&) {
        // Already shutdown or not initialized
    }
}

std::shared_ptr<spdlog::logger> StructuredLogger::getLogger() const {
    return logger_;
}

void StructuredLogger::setLevel(spdlog::level::level_enum level) {
    config_.level = level;
    if (logger_) {
        logger_->set_level(level);
    }
}

spdlog::level::level_enum StructuredLogger::getLevel() const {
    return config_.level;
}

// PerformanceTimer implementation
PerformanceTimer::PerformanceTimer(const std::string& operation_name,
                                 const std::unordered_map<std::string, std::string>& context)
    : operation_name_(operation_name), context_(context), stopped_(false) {
    start_time_ = std::chrono::high_resolution_clock::now();
}

PerformanceTimer::~PerformanceTimer() {
    if (!stopped_) {
        stop();
    }
}

void PerformanceTimer::addContext(const std::string& key, const std::string& value) {
    context_[key] = value;
}

void PerformanceTimer::addMetric(const std::string& key, double value) {
    metrics_[key] = value;
}

void PerformanceTimer::stop() {
    if (stopped_) return;
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
    double duration_ms = duration.count() / 1000.0;
    
    StructuredLogger::getInstance().logPerformance(operation_name_, duration_ms, context_, metrics_);
    stopped_ = true;
}

} // namespace logservices
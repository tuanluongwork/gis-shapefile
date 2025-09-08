#include "pxpoint_logger.h"
#include <spdlog/sinks/daily_file_sink.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

namespace pxpoint {

PxPointLogger& PxPointLogger::getInstance() {
    static PxPointLogger instance;
    return instance;
}

void PxPointLogger::initialize(const std::string& process_type, spdlog::level::level_enum log_level) {
    if (initialized_) {
        return;
    }
    
    process_type_ = process_type;
    
    // Ensure log directory exists
    std::filesystem::create_directories("/tmp/pxpoint-logs");
    
    // Create log filename with process type and timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream filename;
    filename << "/tmp/pxpoint-logs/pxpoint-" << process_type << "-" << time_t << ".log";
    
    try {
        // Initialize async thread pool if not already done
        try {
            spdlog::init_thread_pool(8192, 1); // 8192 slots, 1 thread
        } catch (const std::exception&) {
            // Thread pool already initialized
        }
        
        // Create console sink
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%l] [%n] %v");
        
        // Create daily file sink
        auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
            filename.str(), 0, 0); // No rotation by hour/minute, just daily
        file_sink->set_level(spdlog::level::debug);
        
        // Create JSON pattern for file logging
        file_sink->set_pattern(
            R"({"timestamp":"%Y-%m-%dT%H:%M:%S.%fZ","level":"%l","process":"%n","message":"%v"})");
        
        // Create async logger
        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        logger_ = std::make_shared<spdlog::async_logger>(
            process_type, 
            sinks.begin(), 
            sinks.end(),
            spdlog::thread_pool(),
            spdlog::async_overflow_policy::block
        );
        
        logger_->set_level(log_level);
        spdlog::register_logger(logger_);
        
        initialized_ = true;
        
        // Log initialization
        logWithContext(spdlog::level::info, "Logger", "PxPoint logger initialized", 
                      {{"process_type", process_type}, {"log_file", filename.str()}});
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to initialize PxPoint logger: " + std::string(e.what()));
    }
}

std::string PxPointLogger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return ss.str();
}

std::string PxPointLogger::formatStructuredMessage(
    const std::string& message,
    const std::unordered_map<std::string, std::string>& context,
    const std::unordered_map<std::string, double>& performance) {
    
    std::stringstream ss;
    ss << message;
    
    // Get correlation information
    auto& correlation_manager = PxPointCorrelationManager::getInstance();
    std::string full_correlation = correlation_manager.getFullCorrelationId();
    
    if (!full_correlation.empty() || !context.empty() || !performance.empty()) {
        ss << " | ";
        
        // Add correlation information
        if (!full_correlation.empty()) {
            ss << "correlation:" << full_correlation;
        }
        
        // Add context fields
        for (const auto& [key, value] : context) {
            ss << " " << key << ":" << value;
        }
        
        // Add performance fields
        for (const auto& [key, value] : performance) {
            ss << " " << key << ":" << std::fixed << std::setprecision(2) << value;
        }
    }
    
    return ss.str();
}

void PxPointLogger::logWithContext(spdlog::level::level_enum level,
                                  const std::string& component,
                                  const std::string& message,
                                  const std::unordered_map<std::string, std::string>& context,
                                  const std::unordered_map<std::string, double>& performance) {
    if (!initialized_) {
        throw std::runtime_error("PxPoint logger not initialized");
    }
    
    std::string structured_message = formatStructuredMessage(message, context, performance);
    
    // Add component to the structured message
    std::stringstream final_message;
    final_message << "[" << component << "] " << structured_message;
    
    logger_->log(level, final_message.str());
}

void PxPointLogger::logProcessStart(const std::string& process_type,
                                   const std::unordered_map<std::string, std::string>& config) {
    auto context = config;
    context["event_type"] = "process_start";
    context["timestamp"] = getCurrentTimestamp();
    
    logWithContext(spdlog::level::info, "Process", 
                  "Process started: " + process_type, context);
}

void PxPointLogger::logProcessEnd(const std::string& process_type, bool success,
                                 const std::unordered_map<std::string, double>& metrics) {
    std::unordered_map<std::string, std::string> context;
    context["event_type"] = "process_end";
    context["success"] = success ? "true" : "false";
    context["timestamp"] = getCurrentTimestamp();
    
    auto log_level = success ? spdlog::level::info : spdlog::level::err;
    std::string message = success ? "Process completed successfully: " : "Process failed: ";
    message += process_type;
    
    logWithContext(log_level, "Process", message, context, metrics);
}

void PxPointLogger::logActivityStart(const std::string& activity_name,
                                    const std::unordered_map<std::string, std::string>& context) {
    auto activity_context = context;
    activity_context["event_type"] = "activity_start";
    activity_context["timestamp"] = getCurrentTimestamp();
    
    logWithContext(spdlog::level::debug, "Activity", 
                  "Activity started: " + activity_name, activity_context);
}

void PxPointLogger::logActivityEnd(const std::string& activity_name, bool success,
                                  const std::unordered_map<std::string, double>& metrics) {
    std::unordered_map<std::string, std::string> context;
    context["event_type"] = "activity_end";
    context["success"] = success ? "true" : "false";
    context["timestamp"] = getCurrentTimestamp();
    
    auto log_level = success ? spdlog::level::debug : spdlog::level::warn;
    std::string message = success ? "Activity completed: " : "Activity failed: ";
    message += activity_name;
    
    logWithContext(log_level, "Activity", message, context, metrics);
}

void PxPointLogger::logError(const std::string& component, const std::string& message,
                            const std::string& exception,
                            const std::unordered_map<std::string, std::string>& context) {
    auto error_context = context;
    error_context["event_type"] = "error";
    error_context["timestamp"] = getCurrentTimestamp();
    if (!exception.empty()) {
        error_context["exception"] = exception;
    }
    
    std::string error_message = "ERROR: " + message;
    if (!exception.empty()) {
        error_message += " | Exception: " + exception;
    }
    
    logWithContext(spdlog::level::err, component, error_message, error_context);
}

void PxPointLogger::shutdown() {
    if (logger_) {
        logger_->flush();
        spdlog::drop(logger_->name());
    }
    
    // Shutdown thread pool
    try {
        spdlog::shutdown();
    } catch (const std::exception&) {
        // Already shutdown or not initialized
    }
}

} // namespace pxpoint
#include "logger.h"
#include "correlation_id.h"
#include <spdlog/pattern_formatter.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <yaml-cpp/yaml.h>
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <filesystem>
#include <fstream>

namespace gis {

// Custom sink that combines daily rotation with size limits
class hybrid_file_sink : public spdlog::sinks::base_sink<std::mutex> {
private:
    std::string base_filename_;
    std::string current_filename_;
    std::FILE* file_;
    size_t max_size_;
    size_t current_size_;
    int file_counter_;
    std::tm last_date_;
    
    void rotate_if_needed() {
        auto now = std::time(nullptr);
        auto tm_now = *std::localtime(&now);
        
        bool day_changed = (tm_now.tm_year != last_date_.tm_year ||
                           tm_now.tm_mon != last_date_.tm_mon ||
                           tm_now.tm_mday != last_date_.tm_mday);
        
        // Always create file if we don't have one yet
        bool need_new_file = (!file_) || day_changed || (current_size_ >= max_size_);
        
        if (need_new_file) {
            if (file_) {
                std::fclose(file_);
                file_ = nullptr;
            }
            
            if (day_changed) {
                file_counter_ = 0;
                last_date_ = tm_now;
            } else {
                file_counter_++;
            }
            
            // Create new filename with date and counter
            std::ostringstream oss;
            oss << base_filename_ << "."
                << std::put_time(&tm_now, "%Y-%m-%d");
            if (file_counter_ > 0) {
                oss << "." << file_counter_;
            }
            current_filename_ = oss.str();
            
            file_ = std::fopen(current_filename_.c_str(), "a");
            if (!file_) {
                throw spdlog::spdlog_ex("Failed to open " + current_filename_ + " for writing");
            }
            current_size_ = std::ftell(file_);
        }
    }

public:
    hybrid_file_sink(const std::string& base_filename, size_t max_size)
        : base_filename_(base_filename), file_(nullptr), max_size_(max_size), 
          current_size_(0), file_counter_(0) {
        auto now = std::time(nullptr);
        last_date_ = *std::localtime(&now);
        
        // Create directory if it doesn't exist
        std::filesystem::path file_path(base_filename_);
        if (file_path.has_parent_path()) {
            std::filesystem::create_directories(file_path.parent_path());
        }
        
        rotate_if_needed();
    }
    
    ~hybrid_file_sink() {
        if (file_) {
            std::fclose(file_);
        }
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        rotate_if_needed();
        
        if (file_) {
            spdlog::memory_buf_t formatted;
            formatter_->format(msg, formatted);
            std::fwrite(formatted.data(), 1, formatted.size(), file_);
            std::fflush(file_);
            current_size_ += formatted.size();
        }
    }

    void flush_() override {
        if (file_) {
            std::fflush(file_);
        }
    }
};


Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::initialize(const std::string& config_file) {
    if (initialized_) {
        return;
    }
    
    try {
        loadConfig(config_file);
        
        // Get configuration values
        auto logging_config = config_["logging"];
        std::string log_level = logging_config["level"].as<std::string>("info");
        std::string log_file = logging_config["file"].as<std::string>("logs/gis-server.log");
        size_t max_file_size = logging_config["max_file_size"].as<size_t>(5 * 1024 * 1024);
        // size_t max_files = logging_config["max_files"].as<size_t>(3);
        std::string pattern = logging_config["pattern"].as<std::string>(
            R"({"timestamp":"%Y-%m-%dT%H:%M:%S.%fZ","level":"%l","logger":"%n","message":"%v"})");
        
        // Create sinks
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);
        
        auto file_sink = std::make_shared<hybrid_file_sink>(log_file, max_file_size);
        file_sink->set_level(spdlog::level::trace);
        
        // Create default logger with both sinks
        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        auto default_logger = std::make_shared<spdlog::logger>("default", sinks.begin(), sinks.end());
        
        // Set pattern from config
        default_logger->set_pattern(pattern);
        
        // Set log level from config
        default_logger->set_level(parseLogLevel(log_level));
        
        // Enable automatic flushing
        default_logger->flush_on(spdlog::level::info);
        spdlog::flush_every(std::chrono::seconds(1));
        
        spdlog::register_logger(default_logger);
        spdlog::set_default_logger(default_logger);
        
        loggers_["default"] = default_logger;
        initialized_ = true;
        
        // Log initialization success
        default_logger->info("Logger initialized successfully from config: {}", config_file);
        
    } catch (const std::exception& ex) {
        std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
        // Fall back to default initialization
        initialize("info", "logs/gis-server.log");
    }
}

void Logger::initialize(const std::string& log_level, 
                       const std::string& log_file,
                       size_t max_file_size,
                       size_t /* max_files */) {
    if (initialized_) {
        return;
    }
    
    try {
        // Create console sink
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);
        
        // Create file sink with daily rotation
        auto file_sink = std::make_shared<hybrid_file_sink>(log_file, max_file_size);
        file_sink->set_level(spdlog::level::trace);
        
        // Create default logger with both sinks
        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        auto default_logger = std::make_shared<spdlog::logger>("default", sinks.begin(), sinks.end());
        
        // Set JSON-like pattern for structured logging
        default_logger->set_pattern(R"({"timestamp":"%Y-%m-%dT%H:%M:%S.%fZ","level":"%l","logger":"%n","message":"%v"})");
        
        // Set log level
        if (log_level == "trace") {
            default_logger->set_level(spdlog::level::trace);
        } else if (log_level == "debug") {
            default_logger->set_level(spdlog::level::debug);
        } else if (log_level == "info") {
            default_logger->set_level(spdlog::level::info);
        } else if (log_level == "warn") {
            default_logger->set_level(spdlog::level::warn);
        } else if (log_level == "error") {
            default_logger->set_level(spdlog::level::err);
        } else if (log_level == "critical") {
            default_logger->set_level(spdlog::level::critical);
        }
        
        spdlog::register_logger(default_logger);
        spdlog::set_default_logger(default_logger);
        
        // Enable automatic flushing to ensure logs are written to file immediately
        default_logger->flush_on(spdlog::level::info);
        spdlog::flush_every(std::chrono::seconds(1));
        
        loggers_["default"] = default_logger;
        initialized_ = true;
        
        // Log initialization success
        default_logger->info("Logger initialized successfully");
        
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
    }
}

std::shared_ptr<spdlog::logger> Logger::getLogger(const std::string& name) {
    if (!initialized_) {
        initialize();
    }
    
    if (loggers_.find(name) == loggers_.end()) {
        // Create new logger with same sinks as default
        auto default_logger = loggers_["default"];
        auto new_logger = std::make_shared<spdlog::logger>(name, default_logger->sinks().begin(), default_logger->sinks().end());
        
        // Check for component-specific configuration
        if (config_["logging"]["loggers"] && config_["logging"]["loggers"][name]) {
            std::string component_level = config_["logging"]["loggers"][name]["level"].as<std::string>();
            new_logger->set_level(parseLogLevel(component_level));
        } else {
            new_logger->set_level(default_logger->level());
        }
        
        // Set the same pattern as the default logger (get pattern from config)
        std::string pattern = config_["logging"]["pattern"].as<std::string>(
            R"({"timestamp":"%Y-%m-%dT%H:%M:%S.%fZ","level":"%l","logger":"%n","message":"%v"})");
        new_logger->set_pattern(pattern);
        new_logger->flush_on(spdlog::level::info);
        
        spdlog::register_logger(new_logger);
        loggers_[name] = new_logger;
    }
    
    return loggers_[name];
}


void Logger::logWithContext(spdlog::level::level_enum level,
                           const std::string& logger_name,
                           const std::string& message,
                           const std::unordered_map<std::string, std::string>& context,
                           const std::unordered_map<std::string, double>& performance) {
    auto logger = getLogger(logger_name);
    std::string structured_message = formatStructuredMessage(message, context, performance);
    logger->log(level, structured_message);
}

std::string Logger::formatStructuredMessage(const std::string& message,
                                          const std::unordered_map<std::string, std::string>& context,
                                          const std::unordered_map<std::string, double>& performance) {
    std::stringstream ss;
    ss << message;
    
    if (!context.empty() || !performance.empty()) {
        ss << " | ";
        
        // Add correlation ID
        ss << "correlation_id:" << CorrelationIdManager::getInstance().getCorrelationId();
        
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

void Logger::loadConfig(const std::string& config_file) {
    if (!std::filesystem::exists(config_file)) {
        throw std::runtime_error("Configuration file not found: " + config_file);
    }
    
    config_ = YAML::LoadFile(config_file);
    
    if (!config_["logging"]) {
        throw std::runtime_error("Invalid configuration: missing 'logging' section");
    }
}

spdlog::level::level_enum Logger::parseLogLevel(const std::string& level_str) {
    if (level_str == "trace") return spdlog::level::trace;
    if (level_str == "debug") return spdlog::level::debug;
    if (level_str == "info") return spdlog::level::info;
    if (level_str == "warn" || level_str == "warning") return spdlog::level::warn;
    if (level_str == "error") return spdlog::level::err;
    if (level_str == "critical") return spdlog::level::critical;
    
    // Default to info if unknown
    return spdlog::level::info;
}

} // namespace gis
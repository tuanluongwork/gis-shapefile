#include "structured_logger.h"
#include "correlation_manager.h"
#include <iostream>
#include <thread>

using namespace logservices;

void demonstrate_custom_configuration() {
    std::cout << "=== Custom Configuration Example ===" << std::endl;
    
    // Create custom logger configuration
    LoggerConfig config;
    config.name = "custom-app";
    config.level = spdlog::level::debug;
    config.log_directory = "./custom-logs";
    config.async_logging = false;  // Use sync for immediate output
    config.auto_add_correlation = true;
    
    // Configure custom sinks
    config.sinks.clear();
    
    // Custom console sink with specific pattern
    SinkConfig console_sink;
    console_sink.type = SinkConfig::Type::Console;
    console_sink.name = "custom_console";
    console_sink.level = spdlog::level::info;
    console_sink.pattern = "[CUSTOM] %Y-%m-%d %H:%M:%S.%f [%^%=8l%$] %v";
    console_sink.color_mode = true;
    config.sinks.push_back(console_sink);
    
    // Custom rotating file sink
    SinkConfig file_sink;
    file_sink.type = SinkConfig::Type::RotatingFile;
    file_sink.name = "custom_file";
    file_sink.level = spdlog::level::debug;
    file_sink.pattern = R"({"app":"custom","timestamp":"%Y-%m-%dT%H:%M:%S.%fZ","level":"%^%l%$","thread":"%t","message":"%v"})";
    file_sink.file_path = "./custom-logs/{}-custom.log";
    file_sink.max_file_size = 1024 * 1024; // 1MB
    file_sink.max_files = 3;
    config.sinks.push_back(file_sink);
    
    // Configure the logger
    auto& logger = StructuredLogger::getInstance();
    logger.configure(config);
    logger.initialize("custom-example");
    
    LOG_INFO("Custom logger initialized with specific configuration");
}

void demonstrate_custom_correlation() {
    std::cout << "\n=== Custom Correlation Configuration ===" << std::endl;
    
    // Create custom correlation configuration
    CorrelationConfig corr_config;
    corr_config.pipeline_id_prefix = "custom-pipeline";
    corr_config.process_id_prefix = "custom-proc";
    corr_config.activity_id_prefix = "custom-act";
    corr_config.env_var_pipeline = "CUSTOM_PIPELINE_ID";
    corr_config.env_var_process = "CUSTOM_PROCESS_ID";
    
    // Set custom ID generators
    corr_config.pipeline_id_generator = []() {
        return "custom-pipeline-" + std::to_string(std::time(nullptr));
    };
    
    corr_config.process_id_generator = [](const std::string& process_type) {
        return "custom-proc-" + process_type + "-" + std::to_string(std::rand() % 10000);
    };
    
    corr_config.activity_id_generator = [](const std::string& activity_name) {
        return "custom-act-" + activity_name + "-" + std::to_string(std::rand() % 1000);
    };
    
    // Configure correlation manager
    auto& correlation = CorrelationManager::getInstance();
    correlation.configure(corr_config);
    
    // Use custom correlation
    ProcessScope process_scope("custom-process");
    
    LOG_INFO("Custom correlation initialized", {
        {"pipeline_id", correlation.getPipelineId()},
        {"process_id", correlation.getProcessId()}
    });
    
    // Test custom activity correlation
    {
        ActivityScope activity("custom-activity");
        LOG_INFO("Inside custom activity scope", {
            {"activity_id", activity.getActivityId()}
        });
        
        {
            ActivityScope nested_activity("nested-activity");
            LOG_INFO("Inside nested custom activity", {
                {"nested_activity_id", nested_activity.getActivityId()}
            });
        }
        
        LOG_INFO("Back in outer activity scope");
    }
    
    LOG_INFO("Custom correlation demonstration completed");
}

void demonstrate_custom_formatting() {
    std::cout << "\n=== Custom Formatting Example ===" << std::endl;
    
    // Create logger config with custom formatter
    LoggerConfig config;
    config.name = "custom-format-app";
    config.log_directory = "./custom-logs";
    config.auto_add_correlation = false;  // We'll handle correlation manually
    
    // Custom formatter that creates XML-style output
    config.custom_formatter = [](const std::string& message, 
                                 const std::unordered_map<std::string, std::string>& context) {
        std::stringstream ss;
        ss << "<log>";
        ss << "<message>" << message << "</message>";
        
        if (!context.empty()) {
            ss << "<context>";
            for (const auto& [key, value] : context) {
                ss << "<" << key << ">" << value << "</" << key << ">";
            }
            ss << "</context>";
        }
        
        // Add correlation manually
        auto& correlation = CorrelationManager::getInstance();
        auto correlation_context = correlation.getCorrelationContext();
        if (!correlation_context.empty()) {
            ss << "<correlation>";
            for (const auto& [key, value] : correlation_context) {
                ss << "<" << key << ">" << value << "</" << key << ">";
            }
            ss << "</correlation>";
        }
        
        ss << "</log>";
        return ss.str();
    };
    
    // Configure console sink with simple pattern since we're doing custom formatting
    SinkConfig console_sink;
    console_sink.type = SinkConfig::Type::Console;
    console_sink.name = "xml_console";
    console_sink.level = spdlog::level::info;
    console_sink.pattern = "%Y-%m-%d %H:%M:%S.%f [%^%l%$] %v";  // The %v will contain our XML
    config.sinks.push_back(console_sink);
    
    auto& logger = StructuredLogger::getInstance();
    logger.configure(config);
    logger.initialize("xml-formatter");
    
    ProcessScope process_scope("xml-demo");
    
    LOG_INFO("Testing XML-style custom formatting", {
        {"feature", "custom_formatter"},
        {"output_format", "xml"}
    });
    
    {
        ActivityScope activity("xml-formatting-test");
        
        LOG_INFO("Message with rich context", {
            {"user_id", "12345"},
            {"operation", "data_processing"},
            {"status", "success"},
            {"processing_time_ms", "150.5"}
        });
    }
    
    LOG_WARN("Warning with custom formatting", {
        {"warning_type", "performance"},
        {"threshold_exceeded", "true"}
    });
}

void demonstrate_runtime_configuration() {
    std::cout << "\n=== Runtime Configuration Changes ===" << std::endl;
    
    auto& logger = StructuredLogger::getInstance();
    
    LOG_INFO("Initial log level test - this should appear");
    LOG_DEBUG("Initial debug test - this might not appear depending on level");
    
    // Change log level at runtime
    std::cout << "Changing log level to DEBUG..." << std::endl;
    logger.setLevel(spdlog::level::debug);
    
    LOG_DEBUG("Debug message after level change - this should now appear");
    LOG_INFO("Info message after level change");
    
    // Change back to INFO
    std::cout << "Changing log level back to INFO..." << std::endl;
    logger.setLevel(spdlog::level::info);
    
    LOG_DEBUG("Debug message after changing back - this should not appear");
    LOG_INFO("Info message after changing back - this should appear");
    
    // Test different log levels
    LOG_INFO("Testing different log levels...");
    LOG_WARN("This is a warning");
    LOG_ERROR("This is an error");
    LOG_CRITICAL("This is critical");
    
    LOG_INFO("Runtime configuration demonstration completed");
}

int main() {
    try {
        demonstrate_custom_configuration();
        demonstrate_custom_correlation();
        demonstrate_custom_formatting();
        demonstrate_runtime_configuration();
        
        std::cout << "\n=== Custom configuration examples completed ===" << std::endl;
        std::cout << "Check ./custom-logs/ directory for custom formatted output" << std::endl;
        
        // Ensure all logs are written
        StructuredLogger::getInstance().flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
    } catch (const std::exception& e) {
        std::cerr << "Custom configuration example failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
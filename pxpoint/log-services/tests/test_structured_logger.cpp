#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "structured_logger.h"
#include "correlation_manager.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

using namespace logservices;

class LoggerTestFixture {
public:
    LoggerTestFixture() {
        // Create test log directory
        test_log_dir = "./test-logs";
        std::filesystem::create_directories(test_log_dir);
    }
    
    ~LoggerTestFixture() {
        // Clean up test logs
        try {
            std::filesystem::remove_all(test_log_dir);
        } catch (...) {
            // Ignore cleanup errors
        }
    }
    
    std::string test_log_dir;
};

TEST_CASE_METHOD(LoggerTestFixture, "StructuredLogger basic initialization", "[logger]") {
    auto& logger = StructuredLogger::getInstance();
    
    SECTION("Initialize with default configuration") {
        REQUIRE_NOTHROW(logger.initialize("test-process"));
        REQUIRE(logger.getLevel() == spdlog::level::info);
    }
    
    SECTION("Initialize with custom log level") {
        REQUIRE_NOTHROW(logger.initialize("test-process", spdlog::level::debug));
        REQUIRE(logger.getLevel() == spdlog::level::debug);
    }
}

TEST_CASE_METHOD(LoggerTestFixture, "StructuredLogger custom configuration", "[logger][config]") {
    auto& logger = StructuredLogger::getInstance();
    
    SECTION("Configure with custom settings") {
        LoggerConfig config;
        config.name = "test-logger";
        config.level = spdlog::level::warn;
        config.log_directory = test_log_dir;
        config.async_logging = false;  // Sync for testing
        config.auto_add_correlation = false;
        
        // Single console sink for testing
        config.sinks.clear();
        SinkConfig console_sink;
        console_sink.type = SinkConfig::Type::Console;
        console_sink.name = "test_console";
        console_sink.level = spdlog::level::warn;
        console_sink.pattern = "[TEST] %v";
        config.sinks.push_back(console_sink);
        
        logger.configure(config);
        REQUIRE_NOTHROW(logger.initialize("config-test"));
        
        REQUIRE(logger.getLevel() == spdlog::level::warn);
    }
    
    SECTION("Configure with file sink") {
        LoggerConfig config;
        config.name = "file-test-logger";
        config.log_directory = test_log_dir;
        config.async_logging = false;
        
        config.sinks.clear();
        SinkConfig file_sink;
        file_sink.type = SinkConfig::Type::File;
        file_sink.name = "test_file";
        file_sink.level = spdlog::level::debug;
        file_sink.pattern = "%v";
        file_sink.file_path = test_log_dir + "/{}-test.log";
        config.sinks.push_back(file_sink);
        
        logger.configure(config);
        REQUIRE_NOTHROW(logger.initialize("file-test"));
        
        // Test that file is created
        logger.info("Test file message");
        logger.flush();
        
        std::string expected_file = test_log_dir + "/file-test-test.log";
        REQUIRE(std::filesystem::exists(expected_file));
    }
}

TEST_CASE_METHOD(LoggerTestFixture, "StructuredLogger basic logging", "[logger][logging]") {
    auto& logger = StructuredLogger::getInstance();
    
    LoggerConfig config;
    config.async_logging = false;  // Sync for testing
    config.log_directory = test_log_dir;
    logger.configure(config);
    logger.initialize("basic-test");
    
    SECTION("Basic log methods") {
        REQUIRE_NOTHROW(logger.debug("Debug message"));
        REQUIRE_NOTHROW(logger.info("Info message"));
        REQUIRE_NOTHROW(logger.warn("Warning message"));
        REQUIRE_NOTHROW(logger.error("Error message"));
        REQUIRE_NOTHROW(logger.critical("Critical message"));
    }
    
    SECTION("Log with context") {
        std::unordered_map<std::string, std::string> context = {
            {"user_id", "12345"},
            {"operation", "test"},
            {"status", "success"}
        };
        
        REQUIRE_NOTHROW(logger.info("Test message with context", context));
    }
    
    SECTION("Component-specific logging") {
        REQUIRE_NOTHROW(logger.info("TestComponent", "Component message"));
        REQUIRE_NOTHROW(logger.error("ErrorComponent", "Error message", {{"error_code", "404"}}));
    }
    
    SECTION("Log with metrics") {
        std::unordered_map<std::string, double> metrics = {
            {"duration_ms", 123.45},
            {"memory_usage_mb", 256.0},
            {"cpu_percent", 75.5}
        };
        
        REQUIRE_NOTHROW(logger.log(spdlog::level::info, "Performance data", {}, metrics));
    }
}

TEST_CASE_METHOD(LoggerTestFixture, "StructuredLogger event logging", "[logger][events]") {
    auto& logger = StructuredLogger::getInstance();
    
    LoggerConfig config;
    config.async_logging = false;
    config.log_directory = test_log_dir;
    logger.configure(config);
    logger.initialize("event-test");
    
    SECTION("Log structured events") {
        REQUIRE_NOTHROW(logger.logEvent("user_action", "Button clicked", 
            {{"button_id", "submit"}, {"user_id", "123"}}));
        
        REQUIRE_NOTHROW(logger.logEvent("system_event", "Cache cleared", 
            {{"cache_type", "session"}}, {{"entries_cleared", 100.0}}));
    }
    
    SECTION("Process lifecycle events") {
        REQUIRE_NOTHROW(logger.logProcessStart("test-process", {{"version", "1.0"}}));
        REQUIRE_NOTHROW(logger.logProcessEnd("test-process", true, {{"duration_ms", 1000.0}}));
        REQUIRE_NOTHROW(logger.logProcessEnd("test-process", false));
    }
    
    SECTION("Activity lifecycle events") {
        REQUIRE_NOTHROW(logger.logActivityStart("test-activity", {{"batch_id", "batch_001"}}));
        REQUIRE_NOTHROW(logger.logActivityEnd("test-activity", true, {{"items_processed", 50.0}}));
        REQUIRE_NOTHROW(logger.logActivityEnd("test-activity", false));
    }
    
    SECTION("Performance logging") {
        REQUIRE_NOTHROW(logger.logPerformance("database_query", 234.56, 
            {{"query_type", "select"}}, {{"rows_returned", 1000.0}}));
    }
    
    SECTION("Error logging with exceptions") {
        REQUIRE_NOTHROW(logger.logError("Database", "Connection failed"));
        REQUIRE_NOTHROW(logger.logError("Database", "Query failed", "SQL syntax error", 
            {{"query", "SELECT * FROM users"}}));
    }
}

TEST_CASE_METHOD(LoggerTestFixture, "StructuredLogger correlation integration", "[logger][correlation]") {
    auto& logger = StructuredLogger::getInstance();
    auto& correlation = CorrelationManager::getInstance();
    
    LoggerConfig config;
    config.async_logging = false;
    config.log_directory = test_log_dir;
    config.auto_add_correlation = true;
    logger.configure(config);
    logger.initialize("correlation-test");
    
    correlation.reset();
    
    SECTION("Logging with correlation context") {
        ProcessScope process_scope("correlation-test-process");
        
        // Set up correlation
        REQUIRE_FALSE(correlation.getPipelineId().empty());
        REQUIRE_FALSE(correlation.getProcessId().empty());
        
        // Log message - correlation should be automatically added
        REQUIRE_NOTHROW(logger.info("Message with correlation"));
        
        {
            ActivityScope activity("test-activity");
            REQUIRE_NOTHROW(logger.info("Message with activity correlation"));
        }
    }
    
    SECTION("Disable automatic correlation") {
        config.auto_add_correlation = false;
        logger.configure(config);
        logger.initialize("no-correlation-test");
        
        ProcessScope process_scope("no-correlation-process");
        REQUIRE_NOTHROW(logger.info("Message without auto-correlation"));
    }
}

TEST_CASE_METHOD(LoggerTestFixture, "StructuredLogger performance timer", "[logger][performance]") {
    auto& logger = StructuredLogger::getInstance();
    
    LoggerConfig config;
    config.async_logging = false;
    config.log_directory = test_log_dir;
    logger.configure(config);
    logger.initialize("perf-test");
    
    SECTION("Performance timer basic usage") {
        {
            PerformanceTimer timer("test_operation");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            // Timer destructor should log performance automatically
        }
    }
    
    SECTION("Performance timer with context and metrics") {
        {
            PerformanceTimer timer("complex_operation", {{"type", "batch_processing"}});
            timer.addContext("batch_id", "batch_123");
            timer.addMetric("items_count", 1000.0);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            timer.stop();  // Explicit stop
        }
    }
    
    SECTION("Performance timer manual stop") {
        PerformanceTimer timer("manual_operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        
        REQUIRE_NOTHROW(timer.stop());
        REQUIRE_NOTHROW(timer.stop());  // Should be safe to call multiple times
    }
}

TEST_CASE_METHOD(LoggerTestFixture, "StructuredLogger level management", "[logger][levels]") {
    auto& logger = StructuredLogger::getInstance();
    
    LoggerConfig config;
    config.async_logging = false;
    config.log_directory = test_log_dir;
    logger.configure(config);
    logger.initialize("level-test", spdlog::level::info);
    
    SECTION("Initial level") {
        REQUIRE(logger.getLevel() == spdlog::level::info);
    }
    
    SECTION("Change level at runtime") {
        logger.setLevel(spdlog::level::debug);
        REQUIRE(logger.getLevel() == spdlog::level::debug);
        
        logger.setLevel(spdlog::level::warn);
        REQUIRE(logger.getLevel() == spdlog::level::warn);
    }
    
    SECTION("Level affects logging") {
        logger.setLevel(spdlog::level::warn);
        
        // These should not throw, but may not produce output depending on level
        REQUIRE_NOTHROW(logger.debug("This debug message should be filtered"));
        REQUIRE_NOTHROW(logger.info("This info message should be filtered"));
        REQUIRE_NOTHROW(logger.warn("This warning should appear"));
        REQUIRE_NOTHROW(logger.error("This error should appear"));
    }
}

TEST_CASE_METHOD(LoggerTestFixture, "StructuredLogger thread safety", "[logger][threading]") {
    auto& logger = StructuredLogger::getInstance();
    
    LoggerConfig config;
    config.async_logging = true;  // Enable async for thread safety test
    config.log_directory = test_log_dir;
    logger.configure(config);
    logger.initialize("thread-test");
    
    SECTION("Concurrent logging from multiple threads") {
        const int num_threads = 4;
        const int messages_per_thread = 100;
        std::vector<std::thread> threads;
        
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([t, messages_per_thread, &logger]() {
                for (int i = 0; i < messages_per_thread; ++i) {
                    logger.info("Thread message", {
                        {"thread_id", std::to_string(t)},
                        {"message_id", std::to_string(i)}
                    });
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Flush to ensure all messages are written
        logger.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

TEST_CASE_METHOD(LoggerTestFixture, "StructuredLogger macro usage", "[logger][macros]") {
    auto& logger = StructuredLogger::getInstance();
    
    LoggerConfig config;
    config.async_logging = false;
    config.log_directory = test_log_dir;
    logger.configure(config);
    logger.initialize("macro-test");
    
    SECTION("Basic logging macros") {
        REQUIRE_NOTHROW(LOG_DEBUG("Debug macro test"));
        REQUIRE_NOTHROW(LOG_INFO("Info macro test"));
        REQUIRE_NOTHROW(LOG_WARN("Warning macro test"));
        REQUIRE_NOTHROW(LOG_ERROR("Error macro test"));
        REQUIRE_NOTHROW(LOG_CRITICAL("Critical macro test"));
    }
    
    SECTION("Logging macros with context") {
        REQUIRE_NOTHROW(LOG_INFO("Macro with context", {{"key", "value"}}));
        REQUIRE_NOTHROW(LOG_ERROR("Macro error with context", {{"error_code", "500"}}));
    }
    
    SECTION("Component-specific macros") {
        REQUIRE_NOTHROW(LOG_COMPONENT_INFO("TestComponent", "Component macro test"));
        REQUIRE_NOTHROW(LOG_COMPONENT_ERROR("ErrorComponent", "Component error test", 
            {{"component_version", "1.0"}}));
    }
    
    SECTION("Performance and activity macros") {
        {
            REQUIRE_NOTHROW(LOG_PERFORMANCE_SCOPE("macro_operation", {{"type", "test"}}));
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        {
            REQUIRE_NOTHROW(LOG_ACTIVITY_SCOPE("macro_activity", {{"activity_type", "test"}}));
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

TEST_CASE_METHOD(LoggerTestFixture, "StructuredLogger cleanup and shutdown", "[logger][cleanup]") {
    auto& logger = StructuredLogger::getInstance();
    
    LoggerConfig config;
    config.async_logging = true;
    config.log_directory = test_log_dir;
    logger.configure(config);
    logger.initialize("shutdown-test");
    
    SECTION("Flush operations") {
        logger.info("Message before flush");
        REQUIRE_NOTHROW(logger.flush());
    }
    
    SECTION("Shutdown operations") {
        logger.info("Message before shutdown");
        REQUIRE_NOTHROW(logger.shutdown());
    }
}
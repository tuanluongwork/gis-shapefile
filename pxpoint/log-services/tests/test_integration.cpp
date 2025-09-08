#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "structured_logger.h"
#include "correlation_manager.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <regex>

using namespace logservices;

class IntegrationTestFixture {
public:
    IntegrationTestFixture() {
        test_log_dir = "./integration-test-logs";
        std::filesystem::create_directories(test_log_dir);
        
        // Reset singletons to clean state
        CorrelationManager::getInstance().reset();
    }
    
    ~IntegrationTestFixture() {
        // Clean up
        try {
            StructuredLogger::getInstance().shutdown();
            std::filesystem::remove_all(test_log_dir);
        } catch (...) {
            // Ignore cleanup errors
        }
    }
    
    std::string readLogFile(const std::string& filename) {
        std::string full_path = test_log_dir + "/" + filename;
        std::ifstream file(full_path);
        if (!file.is_open()) {
            return "";
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        return content;
    }
    
    std::string test_log_dir;
};

TEST_CASE_METHOD(IntegrationTestFixture, "End-to-end logging with correlation", "[integration][e2e]") {
    auto& logger = StructuredLogger::getInstance();
    auto& correlation = CorrelationManager::getInstance();
    
    // Configure for file output to verify content
    LoggerConfig config;
    config.name = "integration-test";
    config.level = spdlog::level::debug;
    config.log_directory = test_log_dir;
    config.async_logging = false;  // Sync for immediate file writing
    config.auto_add_correlation = true;
    
    config.sinks.clear();
    SinkConfig file_sink;
    file_sink.type = SinkConfig::Type::File;
    file_sink.name = "integration_file";
    file_sink.level = spdlog::level::debug;
    file_sink.pattern = "%v";  // Just the message for easy parsing
    file_sink.file_path = test_log_dir + "/{}-integration.log";
    config.sinks.push_back(file_sink);
    
    logger.configure(config);
    logger.initialize("e2e-test");
    
    SECTION("Complete workflow with correlation tracking") {
        // Start process scope
        ProcessScope process_scope("integration-workflow");
        
        std::string pipeline_id = correlation.getPipelineId();
        std::string process_id = correlation.getProcessId();
        
        REQUIRE_FALSE(pipeline_id.empty());
        REQUIRE_FALSE(process_id.empty());
        
        // Log process start
        logger.logProcessStart("integration-workflow", {{"version", "1.0.0"}});
        
        // Activity 1: Data validation
        {
            ActivityScope activity("data_validation", {{"batch_id", "batch_001"}});
            
            logger.logActivityStart("data_validation", {{"records_count", "1000"}});
            
            LOG_INFO("Validating data records", {{"validation_type", "schema"}});
            LOG_INFO("Validation step completed", {{"step", "schema_check"}});
            
            logger.logPerformance("validation_step", 150.5, 
                {{"step", "schema_validation"}}, 
                {{"records_processed", 1000.0}});
            
            logger.logActivityEnd("data_validation", true, {{"duration_ms", 150.5}});
        }
        
        // Activity 2: Data processing
        {
            ActivityScope activity("data_processing", {{"algorithm", "standard"}});
            
            logger.logActivityStart("data_processing");
            
            LOG_INFO("Processing data batch", {{"processor", "main_processor"}});
            
            // Simulate error handling
            logger.logError("DataProcessor", "Temporary processing error", 
                "Connection timeout", {{"retry_count", "1"}});
            
            LOG_INFO("Processing completed after retry", {{"status", "success"}});
            
            logger.logActivityEnd("data_processing", true, {{"duration_ms", 300.0}});
        }
        
        // Activity 3: Result aggregation
        {
            LOG_ACTIVITY_SCOPE("result_aggregation", {{"output_format", "json"}});
            
            LOG_INFO("Aggregating results");
            logger.logEvent("results_generated", "Final results generated", 
                {{"output_file", "results.json"}}, 
                {{"total_records", 1000.0}, {"success_rate", 98.5}});
        }
        
        // Log process completion
        logger.logProcessEnd("integration-workflow", true, 
            {{"total_duration_ms", 500.0}, {"records_processed", 1000.0}});
        
        // Ensure all logs are written
        logger.flush();
        
        // Verify log content
        std::string log_content = readLogFile("e2e-test-integration.log");
        REQUIRE_FALSE(log_content.empty());
        
        // Check that correlation IDs appear in logs
        REQUIRE(log_content.find(pipeline_id) != std::string::npos);
        REQUIRE(log_content.find(process_id) != std::string::npos);
        
        // Check that all major log events are present
        REQUIRE(log_content.find("process_start") != std::string::npos);
        REQUIRE(log_content.find("activity_start") != std::string::npos);
        REQUIRE(log_content.find("data_validation") != std::string::npos);
        REQUIRE(log_content.find("data_processing") != std::string::npos);
        REQUIRE(log_content.find("result_aggregation") != std::string::npos);
        REQUIRE(log_content.find("process_end") != std::string::npos);
    }
}

TEST_CASE_METHOD(IntegrationTestFixture, "Multi-process correlation simulation", "[integration][multiprocess]") {
    auto& logger = StructuredLogger::getInstance();
    auto& correlation = CorrelationManager::getInstance();
    
    // Configure logger
    LoggerConfig config;
    config.name = "multiprocess-test";
    config.log_directory = test_log_dir;
    config.async_logging = false;
    config.auto_add_correlation = true;
    
    config.sinks.clear();
    SinkConfig file_sink;
    file_sink.type = SinkConfig::Type::File;
    file_sink.name = "multiprocess_file";
    file_sink.level = spdlog::level::debug;
    file_sink.pattern = "%v";
    file_sink.file_path = test_log_dir + "/{}-multiprocess.log";
    config.sinks.push_back(file_sink);
    
    logger.configure(config);
    
    SECTION("Simulate orchestrator and worker processes") {
        // Simulate orchestrator process
        {
            logger.initialize("orchestrator");
            ProcessScope orchestrator_scope("orchestrator");
            
            std::string shared_pipeline_id = correlation.getPipelineId();
            
            LOG_INFO("Orchestrator starting", {{"workers_to_spawn", "3"}});
            
            // Simulate environment propagation and worker processes
            correlation.saveToEnvironment();
            
            // Simulate worker 1
            {
                // Reset and simulate loading from environment
                std::string saved_pipeline = correlation.getPipelineId();
                std::string saved_process = correlation.getProcessId();
                
                // Simulate new process loading correlation from environment
                correlation.reset();
                correlation.setPipelineId(saved_pipeline);  // Simulate loading from env
                
                ProcessScope worker1_scope("data-validator");
                logger.initialize("worker1");
                
                LOG_INFO("Worker 1 started", {{"worker_type", "data-validator"}});
                
                {
                    LOG_ACTIVITY_SCOPE("validate_schema");
                    LOG_INFO("Schema validation completed");
                }
                
                LOG_INFO("Worker 1 completed");
            }
            
            // Simulate worker 2
            {
                // Reset and reload
                correlation.reset();
                correlation.setPipelineId(shared_pipeline_id);
                
                ProcessScope worker2_scope("geo-processor");
                logger.initialize("worker2");
                
                LOG_INFO("Worker 2 started", {{"worker_type", "geo-processor"}});
                
                {
                    LOG_ACTIVITY_SCOPE("spatial_analysis");
                    LOG_INFO("Spatial analysis completed");
                }
                
                LOG_INFO("Worker 2 completed");
            }
            
            // Back to orchestrator
            correlation.reset();
            correlation.setPipelineId(shared_pipeline_id);
            ProcessScope final_orchestrator_scope("orchestrator");
            logger.initialize("orchestrator-final");
            
            LOG_INFO("All workers completed", {{"total_workers", "2"}});
        }
        
        logger.flush();
        
        // Verify that all processes used the same pipeline ID
        std::string log_content = readLogFile("orchestrator-multiprocess.log");
        log_content += readLogFile("worker1-multiprocess.log");
        log_content += readLogFile("worker2-multiprocess.log");
        log_content += readLogFile("orchestrator-final-multiprocess.log");
        
        REQUIRE_FALSE(log_content.empty());
        
        // Count pipeline ID occurrences to ensure consistency
        std::regex pipeline_regex(R"(pipeline_id[":]+([^",\s}]+))");
        std::sregex_iterator iter(log_content.begin(), log_content.end(), pipeline_regex);
        std::sregex_iterator end;
        
        std::string first_pipeline_id;
        int pipeline_count = 0;
        
        for (; iter != end; ++iter) {
            std::string current_id = (*iter)[1];
            if (first_pipeline_id.empty()) {
                first_pipeline_id = current_id;
            }
            REQUIRE(current_id == first_pipeline_id);  // All should be the same
            pipeline_count++;
        }
        
        REQUIRE(pipeline_count > 0);  // Should find pipeline IDs in logs
    }
}

TEST_CASE_METHOD(IntegrationTestFixture, "Performance under load", "[integration][performance]") {
    auto& logger = StructuredLogger::getInstance();
    
    // Configure for high-performance logging
    LoggerConfig config;
    config.name = "performance-test";
    config.log_directory = test_log_dir;
    config.async_logging = true;  // Enable async for performance
    config.async_queue_size = 16384;
    config.async_thread_count = 2;
    config.auto_add_correlation = true;
    
    config.sinks.clear();
    SinkConfig file_sink;
    file_sink.type = SinkConfig::Type::RotatingFile;
    file_sink.name = "perf_file";
    file_sink.level = spdlog::level::info;
    file_sink.pattern = "%v";
    file_sink.file_path = test_log_dir + "/{}-perf.log";
    file_sink.max_file_size = 1024 * 1024;  // 1MB
    file_sink.max_files = 3;
    config.sinks.push_back(file_sink);
    
    logger.configure(config);
    logger.initialize("perf-test");
    
    SECTION("High-volume logging with correlation") {
        ProcessScope process_scope("performance-test");
        
        const int num_messages = 1000;
        const int num_threads = 4;
        const int messages_per_thread = num_messages / num_threads;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([t, messages_per_thread]() {
                for (int i = 0; i < messages_per_thread; ++i) {
                    ActivityScope activity("perf_activity_" + std::to_string(i % 10));
                    
                    LOG_INFO("Performance test message", {
                        {"thread_id", std::to_string(t)},
                        {"message_id", std::to_string(i)},
                        {"batch_id", std::to_string(i / 100)}
                    });
                    
                    // Occasionally log performance events
                    if (i % 50 == 0) {
                        StructuredLogger::getInstance().logPerformance(
                            "batch_processing", 
                            static_cast<double>(i), 
                            {{"thread", std::to_string(t)}},
                            {{"items_processed", static_cast<double>(i)}}
                        );
                    }
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Ensure all async logs are written
        logger.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Verify performance is reasonable (should complete in under 2 seconds for 1000 messages)
        REQUIRE(duration.count() < 2000);
        
        // Check that log files were created and contain data
        bool log_file_found = false;
        for (const auto& entry : std::filesystem::directory_iterator(test_log_dir)) {
            if (entry.path().filename().string().find("perf-test-perf.log") != std::string::npos) {
                log_file_found = true;
                REQUIRE(std::filesystem::file_size(entry.path()) > 0);
            }
        }
        REQUIRE(log_file_found);
    }
}

TEST_CASE_METHOD(IntegrationTestFixture, "YAML configuration integration", "[integration][yaml]") {
    // Create a test YAML configuration
    std::string yaml_config = R"(
logging:
  name: "yaml-test-app"
  level: "debug"
  log_directory: ")" + test_log_dir + R"("
  async_logging: false
  auto_add_correlation: true
  
  sinks:
    - type: "file"
      name: "yaml_file"
      level: "debug"
      pattern: "%v"
      file_path: ")" + test_log_dir + R"(/{}-yaml-config.log"

correlation:
  pipeline_id_prefix: "yaml-pipeline"
  process_id_prefix: "yaml-proc"
  activity_id_prefix: "yaml-act"
  env_var_pipeline: "YAML_PIPELINE_ID"
  env_var_process: "YAML_PROCESS_ID"
)";
    
    // Write YAML config to file
    std::string config_file = test_log_dir + "/test-config.yaml";
    std::ofstream yaml_file(config_file);
    yaml_file << yaml_config;
    yaml_file.close();
    
    SECTION("Load configuration from YAML") {
        auto& logger = StructuredLogger::getInstance();
        auto& correlation = CorrelationManager::getInstance();
        
        REQUIRE_NOTHROW(correlation.loadConfigFromYaml(config_file));
        REQUIRE_NOTHROW(logger.loadConfigFromYaml(config_file));
        REQUIRE_NOTHROW(logger.initialize("yaml-test"));
        
        ProcessScope process_scope("yaml-configured-process");
        
        // Test that YAML configuration is applied
        std::string pipeline_id = correlation.getPipelineId();
        REQUIRE(pipeline_id.find("yaml-pipeline") != std::string::npos);
        
        std::string process_id = correlation.getProcessId();
        REQUIRE(process_id.find("yaml-proc") != std::string::npos);
        
        // Test logging
        LOG_INFO("YAML configuration test message", {{"config_source", "yaml"}});
        
        {
            ActivityScope activity("yaml-activity");
            std::string activity_id = activity.getActivityId();
            REQUIRE(activity_id.find("yaml-act") != std::string::npos);
            
            LOG_INFO("Message from YAML-configured activity");
        }
        
        logger.flush();
        
        // Verify log file was created with expected content
        std::string log_content = readLogFile("yaml-test-yaml-config.log");
        REQUIRE_FALSE(log_content.empty());
        REQUIRE(log_content.find("YAML configuration test message") != std::string::npos);
        REQUIRE(log_content.find("yaml-configured-process") != std::string::npos);
    }
}

TEST_CASE_METHOD(IntegrationTestFixture, "Error handling and recovery", "[integration][errors]") {
    auto& logger = StructuredLogger::getInstance();
    
    // Configure with intentionally problematic settings to test error handling
    LoggerConfig config;
    config.name = "error-test";
    config.log_directory = test_log_dir;
    config.async_logging = false;
    
    logger.configure(config);
    logger.initialize("error-handling-test");
    
    SECTION("Logging during exception scenarios") {
        ProcessScope process_scope("error-test-process");
        
        // Test logging in various error scenarios
        try {
            throw std::runtime_error("Test exception");
        } catch (const std::exception& e) {
            logger.logError("ExceptionHandler", "Caught test exception", e.what(), 
                {{"exception_type", "runtime_error"}});
        }
        
        // Test logging with extreme values
        std::unordered_map<std::string, double> extreme_metrics = {
            {"very_large_number", 1e20},
            {"very_small_number", 1e-20},
            {"negative_value", -999999.99}
        };
        
        REQUIRE_NOTHROW(logger.logPerformance("extreme_values_test", 0.001, 
            {{"test_type", "extreme_values"}}, extreme_metrics));
        
        // Test logging with special characters
        REQUIRE_NOTHROW(logger.info("Special chars test", {
            {"special_string", "Hello\nWorld\t\"quoted\"\\'single\\"},
            {"unicode_test", "测试中文字符"},
            {"json_like", "{\"key\": \"value\", \"number\": 123}"}
        }));
        
        // Test very long messages
        std::string long_message(1000, 'A');
        REQUIRE_NOTHROW(logger.info("Long message test: " + long_message));
        
        logger.flush();
    }
    
    SECTION("Recovery from logging failures") {
        // Test that the system continues to work even if some operations fail
        
        // Attempt to log with null/empty values
        REQUIRE_NOTHROW(logger.info("", {}));  // Empty message
        REQUIRE_NOTHROW(logger.info("Test", {{"", "empty_key"}}));  // Empty key
        REQUIRE_NOTHROW(logger.info("Test", {{"key", ""}}));  // Empty value
        
        // Multiple rapid flush operations
        for (int i = 0; i < 10; ++i) {
            logger.info("Rapid flush test " + std::to_string(i));
            REQUIRE_NOTHROW(logger.flush());
        }
    }
}
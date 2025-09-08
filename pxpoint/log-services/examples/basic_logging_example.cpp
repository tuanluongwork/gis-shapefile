#include "structured_logger.h"
#include "correlation_manager.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace logservices;

void demonstrate_basic_logging() {
    std::cout << "=== Basic Logging Example ===" << std::endl;
    
    // Initialize logger with default configuration
    auto& logger = StructuredLogger::getInstance();
    logger.initialize("basic-example");
    
    // Basic logging
    LOG_INFO("Application started");
    LOG_DEBUG("Debug information");
    LOG_WARN("This is a warning");
    LOG_ERROR("This is an error");
    
    // Logging with context
    LOG_INFO("Processing user request", {{"user_id", "12345"}, {"operation", "login"}});
    LOG_WARN("High memory usage", {{"memory_usage_mb", "850"}, {"threshold_mb", "800"}});
    
    // Component-specific logging
    LOG_COMPONENT_INFO("Authentication", "User authenticated successfully", 
                      {{"user_id", "12345"}, {"auth_method", "oauth2"}});
    
    LOG_COMPONENT_ERROR("Database", "Connection timeout", 
                       {{"host", "db.example.com"}, {"timeout_ms", "5000"}});
}

void demonstrate_correlation() {
    std::cout << "\n=== Correlation Example ===" << std::endl;
    
    // Set up process correlation
    ProcessScope process_scope("basic-example");
    
    LOG_INFO("Starting process with correlation", {{"process_id", process_scope.getProcessId()}});
    
    // Activity scope with automatic correlation
    {
        LOG_ACTIVITY_SCOPE("user_authentication", {{"user_id", "67890"}});
        
        LOG_INFO("Validating credentials");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        LOG_INFO("Credentials validated successfully");
    }
    
    // Another activity
    {
        ActivityScope activity("data_processing", {{"batch_size", "1000"}});
        
        LOG_INFO("Processing data batch");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        LOG_INFO("Data processing completed");
    }
    
    LOG_INFO("Process completed");
}

void demonstrate_performance_timing() {
    std::cout << "\n=== Performance Timing Example ===" << std::endl;
    
    // Manual performance logging
    auto start = std::chrono::high_resolution_clock::now();
    
    // Simulate work
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double duration_ms = duration.count() / 1000.0;
    
    auto& logger = StructuredLogger::getInstance();
    logger.logPerformance("manual_operation", duration_ms, 
                         {{"operation_type", "data_transform"}}, 
                         {{"records_processed", 1000}, {"throughput_rps", 6666.67}});
    
    // Automatic performance timing with scope
    {
        LOG_PERFORMANCE_SCOPE("automated_operation", {{"operation_type", "data_validation"}});
        
        LOG_INFO("Starting automated operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(75));
        LOG_INFO("Automated operation completed");
        // PerformanceTimer destructor automatically logs the duration
    }
}

void demonstrate_structured_events() {
    std::cout << "\n=== Structured Events Example ===" << std::endl;
    
    auto& logger = StructuredLogger::getInstance();
    
    // Log structured events
    logger.logEvent("user_action", "User clicked button", 
                   {{"button_id", "submit"}, {"page", "checkout"}, {"user_id", "12345"}});
    
    logger.logEvent("system_event", "Cache invalidated", 
                   {{"cache_type", "user_sessions"}, {"reason", "memory_pressure"}}, 
                   {{"entries_cleared", 1500}, {"memory_freed_mb", 45.2}});
    
    // Process lifecycle events
    logger.logProcessStart("data-processor", {{"config_file", "prod.yaml"}, {"version", "1.2.3"}});
    
    // Simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    logger.logProcessEnd("data-processor", true, 
                        {{"records_processed", 5000}, {"processing_time_ms", 100.5}});
}

void demonstrate_error_logging() {
    std::cout << "\n=== Error Logging Example ===" << std::endl;
    
    auto& logger = StructuredLogger::getInstance();
    
    // Simple error
    LOG_ERROR("File not found", {{"file_path", "/tmp/missing.txt"}});
    
    // Error with exception details
    try {
        throw std::runtime_error("Database connection failed");
    } catch (const std::exception& e) {
        logger.logError("Database", "Connection attempt failed", e.what(), 
                       {{"host", "db.example.com"}, {"port", "5432"}, {"retry_count", "3"}});
    }
    
    // Component-specific error
    LOG_COMPONENT_ERROR("PaymentProcessor", "Transaction declined", 
                       {{"transaction_id", "tx_12345"}, {"amount", "99.99"}, {"currency", "USD"}});
}

int main() {
    try {
        demonstrate_basic_logging();
        demonstrate_correlation();
        demonstrate_performance_timing();
        demonstrate_structured_events();
        demonstrate_error_logging();
        
        std::cout << "\n=== Example completed successfully ===" << std::endl;
        std::cout << "Check the log files in /tmp/logs/ for structured output" << std::endl;
        
        // Ensure all logs are written before exit
        StructuredLogger::getInstance().flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
    } catch (const std::exception& e) {
        std::cerr << "Example failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
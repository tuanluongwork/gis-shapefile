#include "structured_logger.h"
#include "correlation_manager.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <random>

using namespace logservices;

void simulate_data_validation() {
    LOG_ACTIVITY_SCOPE("data_validation", {{"batch_id", "batch_001"}});
    
    LOG_INFO("Starting data validation");
    
    // Simulate validation steps
    std::vector<std::string> validation_steps = {
        "schema_validation", 
        "data_integrity_check", 
        "business_rule_validation"
    };
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(50, 150);
    
    for (const auto& step : validation_steps) {
        LOG_INFO("Executing validation step", {{"step", step}});
        
        // Simulate processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));
        
        LOG_INFO("Validation step completed", {{"step", step}});
    }
    
    LOG_INFO("Data validation completed successfully");
}

void simulate_geo_processing() {
    LOG_ACTIVITY_SCOPE("geo_processing", {{"dataset", "parcel_data"}});
    
    LOG_INFO("Starting geo processing");
    
    // Simulate geo processing steps
    std::vector<std::pair<std::string, int>> geo_steps = {
        {"coordinate_transformation", 100},
        {"spatial_indexing", 200},
        {"intersection_analysis", 150}
    };
    
    for (const auto& [step, duration] : geo_steps) {
        LOG_PERFORMANCE_SCOPE(step, {{"algorithm", "rtree"}});
        
        LOG_INFO("Executing geo processing step", {{"step", step}});
        
        // Simulate processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(duration));
        
        LOG_INFO("Geo processing step completed", {{"step", step}});
    }
    
    LOG_INFO("Geo processing completed successfully");
}

void simulate_report_generation() {
    LOG_ACTIVITY_SCOPE("report_generation", {{"report_type", "summary"}});
    
    LOG_INFO("Starting report generation");
    
    // Simulate report generation
    std::vector<std::string> report_sections = {
        "executive_summary",
        "data_statistics", 
        "processing_metrics",
        "quality_assessment"
    };
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(30, 80);
    
    for (const auto& section : report_sections) {
        LOG_INFO("Generating report section", {{"section", section}});
        
        // Simulate generation time
        std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));
        
        LOG_INFO("Report section completed", {{"section", section}});
    }
    
    LOG_INFO("Report generation completed successfully");
}

int main(int argc, char* argv[]) {
    std::string worker_type = "generic-worker";
    if (argc > 1) {
        worker_type = argv[1];
    }
    
    try {
        // Load configuration
        auto& correlation = CorrelationManager::getInstance();
        correlation.loadConfigFromYaml("logging.yaml");
        
        auto& logger = StructuredLogger::getInstance();
        logger.loadConfigFromYaml("logging.yaml");
        logger.initialize(worker_type);
        
        // Initialize process scope - this will load correlation from environment
        ProcessScope process_scope(worker_type);
        
        LOG_INFO("Worker process started", {
            {"worker_type", worker_type}, 
            {"process_id", process_scope.getProcessId()},
            {"pipeline_id", correlation.getPipelineId()}
        });
        
        // Log process start
        logger.logProcessStart(worker_type, {{"version", "1.0.0"}});
        
        // Simulate different types of work based on worker type
        if (worker_type == "data-validator") {
            simulate_data_validation();
        } else if (worker_type == "geo-processor") {
            simulate_geo_processing();
        } else if (worker_type == "report-generator") {
            simulate_report_generation();
        } else {
            LOG_ACTIVITY_SCOPE("generic_work");
            LOG_INFO("Performing generic work");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            LOG_INFO("Generic work completed");
        }
        
        // Log process completion
        logger.logProcessEnd(worker_type, true, {
            {"processing_time_ms", 300.0},
            {"items_processed", 1000.0}
        });
        
        LOG_INFO("Worker process completed successfully", {{"worker_type", worker_type}});
        
        // Flush logs before exit
        logger.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
    } catch (const std::exception& e) {
        LOG_ERROR("Worker process failed", {
            {"worker_type", worker_type}, 
            {"exception", e.what()}
        });
        return 1;
    }
    
    return 0;
}
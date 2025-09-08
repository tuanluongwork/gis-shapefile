#include "structured_logger.h"
#include "correlation_manager.h"
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>

using namespace logservices;

int main() {
    try {
        // Initialize correlation and logging for orchestrator
        auto& correlation = CorrelationManager::getInstance();
        correlation.loadConfigFromYaml("logging.yaml");
        
        auto& logger = StructuredLogger::getInstance();
        logger.loadConfigFromYaml("logging.yaml");
        logger.initialize("orchestrator");
        
        // Start process scope - this will generate pipeline ID
        ProcessScope process_scope("orchestrator");
        
        LOG_INFO("Orchestrator process started", {{"process_id", process_scope.getProcessId()}});
        
        // Log process start
        logger.logProcessStart("orchestrator", {
            {"version", "1.0.0"},
            {"config", "logging.yaml"},
            {"pipeline_id", correlation.getPipelineId()}
        });
        
        // Simulate orchestrating multiple worker processes
        std::vector<std::string> worker_types = {"data-validator", "geo-processor", "report-generator"};
        
        for (const auto& worker_type : worker_types) {
            LOG_ACTIVITY_SCOPE("spawn_worker", {{"worker_type", worker_type}});
            
            LOG_INFO("Spawning worker process", {{"worker_type", worker_type}});
            
            // Prepare command with environment variables for correlation propagation
            std::string command = "./worker_process " + worker_type;
            
            // Execute worker process (correlation will be propagated via environment)
            int result = std::system(command.c_str());
            
            if (result == 0) {
                LOG_INFO("Worker process completed successfully", {{"worker_type", worker_type}});
            } else {
                LOG_ERROR("Worker process failed", {{"worker_type", worker_type}, {"exit_code", std::to_string(result)}});
            }
        }
        
        // Simulate aggregating results
        {
            LOG_ACTIVITY_SCOPE("aggregate_results");
            
            LOG_INFO("Aggregating worker results");
            
            // Simulate some processing time
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            LOG_INFO("Results aggregated successfully", {{"total_workers", std::to_string(worker_types.size())}});
        }
        
        // Log process completion
        logger.logProcessEnd("orchestrator", true, {
            {"workers_spawned", static_cast<double>(worker_types.size())},
            {"total_processing_time_ms", 500.0}
        });
        
        LOG_INFO("Orchestrator process completed successfully");
        
        // Flush logs before exit
        logger.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
    } catch (const std::exception& e) {
        LOG_ERROR("Orchestrator process failed", {{"exception", e.what()}});
        return 1;
    }
    
    return 0;
}
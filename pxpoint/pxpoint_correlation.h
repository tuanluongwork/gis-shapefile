#pragma once

#include <string>
#include <thread>
#include <cstdlib>

namespace pxpoint {

/**
 * Enhanced correlation system for PxPoint multi-process pipeline
 * Supports hierarchical correlation IDs: Pipeline -> Process -> Activity
 */
class PxPointCorrelationManager {
public:
    static PxPointCorrelationManager& getInstance();
    
    // Pipeline-level correlation (shared across all processes)
    void setPipelineId(const std::string& pipeline_id);
    std::string getPipelineId() const;
    
    // Process-level correlation (unique per process type)
    void setProcessId(const std::string& process_id);
    std::string getProcessId() const;
    
    // Activity-level correlation (within process activities)
    void setActivityId(const std::string& activity_id);
    std::string getActivityId() const;
    void clearActivityId();
    
    // Generate new correlation IDs
    std::string generatePipelineId();
    std::string generateProcessId(const std::string& process_type);
    std::string generateActivityId(const std::string& activity_name);
    
    // Environment variable integration for cross-process correlation
    void loadFromEnvironment();
    void saveToEnvironment();
    
    // Get full correlation context for logging
    std::string getFullCorrelationId() const;
    
private:
    PxPointCorrelationManager() = default;
    
    // Thread-local storage for activity-level correlation
    thread_local static std::string thread_activity_id_;
    
    // Process-level correlation (shared across threads in same process)
    std::string pipeline_id_;
    std::string process_id_;
    
    std::string generateUuid();
};

/**
 * RAII scope for activity-level correlation
 */
class ActivityCorrelationScope {
public:
    explicit ActivityCorrelationScope(const std::string& activity_name);
    ~ActivityCorrelationScope();
    
private:
    std::string previous_id_;
};

/**
 * RAII scope for process initialization
 */
class ProcessCorrelationScope {
public:
    explicit ProcessCorrelationScope(const std::string& process_type);
    ~ProcessCorrelationScope();
    
private:
    bool created_new_pipeline_;
};

} // namespace pxpoint
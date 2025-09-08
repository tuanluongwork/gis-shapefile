#pragma once

#include <string>
#include <thread>
#include <memory>
#include <unordered_map>
#include <functional>

namespace logservices {

/**
 * Configuration for correlation behavior
 */
struct CorrelationConfig {
    std::string pipeline_id_prefix = "pipeline";
    std::string process_id_prefix = "proc";
    std::string activity_id_prefix = "act";
    std::string env_var_pipeline = "LOG_PIPELINE_ID";
    std::string env_var_process = "LOG_PROCESS_ID";
    bool auto_generate_pipeline = true;
    bool auto_generate_process = true;
    bool propagate_to_environment = true;
    
    // Custom ID generators
    std::function<std::string()> pipeline_id_generator;
    std::function<std::string(const std::string&)> process_id_generator;
    std::function<std::string(const std::string&)> activity_id_generator;
};

/**
 * Generic correlation manager for multi-process, multi-threaded applications
 * Supports hierarchical correlation IDs: Pipeline -> Process -> Activity
 */
class CorrelationManager {
public:
    static CorrelationManager& getInstance();
    
    // Configuration
    void configure(const CorrelationConfig& config);
    void loadConfigFromYaml(const std::string& yaml_file_path);
    
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
    void saveToEnvironment() const;
    
    // Get correlation context for logging
    std::string getFullCorrelationId() const;
    std::unordered_map<std::string, std::string> getCorrelationContext() const;
    
    // Reset correlation state
    void reset();
    
    // Access configuration (for scope classes)
    const CorrelationConfig& getConfig() const;
    
private:
    CorrelationManager() = default;
    
    // Thread-local storage for activity-level correlation
    thread_local static std::string thread_activity_id_;
    
    // Process-level correlation (shared across threads in same process)
    std::string pipeline_id_;
    std::string process_id_;
    
    // Configuration
    CorrelationConfig config_;
    
    // Default ID generators
    std::string generateDefaultUuid();
    std::string generateTimestampBasedId(const std::string& prefix);
};

/**
 * RAII scope for activity-level correlation
 */
class ActivityScope {
public:
    explicit ActivityScope(const std::string& activity_name);
    explicit ActivityScope(const std::string& activity_name, 
                          const std::unordered_map<std::string, std::string>& context);
    ~ActivityScope();
    
    // Get the activity ID for this scope
    std::string getActivityId() const;
    
    // Add context to the activity
    void addContext(const std::string& key, const std::string& value);
    void addContext(const std::unordered_map<std::string, std::string>& context);
    
private:
    std::string activity_id_;
    std::string previous_id_;
    std::unordered_map<std::string, std::string> context_;
};

/**
 * RAII scope for process initialization
 */
class ProcessScope {
public:
    explicit ProcessScope(const std::string& process_type);
    explicit ProcessScope(const std::string& process_type, 
                         const std::unordered_map<std::string, std::string>& context);
    ~ProcessScope();
    
    // Get the process ID for this scope
    std::string getProcessId() const;
    
private:
    std::string process_id_;
    bool created_new_pipeline_;
    std::unordered_map<std::string, std::string> context_;
};

} // namespace logservices
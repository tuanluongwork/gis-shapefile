#include "correlation_manager.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

namespace logservices {

thread_local std::string CorrelationManager::thread_activity_id_;

CorrelationManager& CorrelationManager::getInstance() {
    static CorrelationManager instance;
    return instance;
}

void CorrelationManager::configure(const CorrelationConfig& config) {
    config_ = config;
}

void CorrelationManager::loadConfigFromYaml(const std::string& yaml_file_path) {
    try {
        YAML::Node config = YAML::LoadFile(yaml_file_path);
        
        if (config["correlation"]) {
            auto corr_config = config["correlation"];
            
            if (corr_config["pipeline_id_prefix"]) {
                config_.pipeline_id_prefix = corr_config["pipeline_id_prefix"].as<std::string>();
            }
            
            if (corr_config["process_id_prefix"]) {
                config_.process_id_prefix = corr_config["process_id_prefix"].as<std::string>();
            }
            
            if (corr_config["activity_id_prefix"]) {
                config_.activity_id_prefix = corr_config["activity_id_prefix"].as<std::string>();
            }
            
            if (corr_config["env_var_pipeline"]) {
                config_.env_var_pipeline = corr_config["env_var_pipeline"].as<std::string>();
            }
            
            if (corr_config["env_var_process"]) {
                config_.env_var_process = corr_config["env_var_process"].as<std::string>();
            }
            
            if (corr_config["auto_generate_pipeline"]) {
                config_.auto_generate_pipeline = corr_config["auto_generate_pipeline"].as<bool>();
            }
            
            if (corr_config["auto_generate_process"]) {
                config_.auto_generate_process = corr_config["auto_generate_process"].as<bool>();
            }
            
            if (corr_config["propagate_to_environment"]) {
                config_.propagate_to_environment = corr_config["propagate_to_environment"].as<bool>();
            }
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to load correlation config from YAML: " + std::string(e.what()));
    }
}

void CorrelationManager::setPipelineId(const std::string& pipeline_id) {
    pipeline_id_ = pipeline_id;
}

std::string CorrelationManager::getPipelineId() const {
    return pipeline_id_;
}

void CorrelationManager::setProcessId(const std::string& process_id) {
    process_id_ = process_id;
}

std::string CorrelationManager::getProcessId() const {
    return process_id_;
}

void CorrelationManager::setActivityId(const std::string& activity_id) {
    thread_activity_id_ = activity_id;
}

std::string CorrelationManager::getActivityId() const {
    return thread_activity_id_;
}

void CorrelationManager::clearActivityId() {
    thread_activity_id_.clear();
}

std::string CorrelationManager::generateDefaultUuid() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; i++) ss << dis(gen);
    ss << "-4"; // Version 4 UUID
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; i++) ss << dis(gen);
    
    return ss.str();
}

std::string CorrelationManager::generateTimestampBasedId(const std::string& prefix) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << prefix << "-" << time_t << "-" << ms.count() << "-" << generateDefaultUuid().substr(0, 8);
    return ss.str();
}

std::string CorrelationManager::generatePipelineId() {
    if (config_.pipeline_id_generator) {
        return config_.pipeline_id_generator();
    }
    
    return generateTimestampBasedId(config_.pipeline_id_prefix);
}

std::string CorrelationManager::generateProcessId(const std::string& process_type) {
    if (config_.process_id_generator) {
        return config_.process_id_generator(process_type);
    }
    
    if (config_.auto_generate_pipeline && pipeline_id_.empty()) {
        pipeline_id_ = generatePipelineId();
    }
    
    std::stringstream ss;
    if (!pipeline_id_.empty()) {
        ss << pipeline_id_ << "-";
    }
    ss << config_.process_id_prefix << "-" << process_type << "-" << generateDefaultUuid().substr(0, 8);
    return ss.str();
}

std::string CorrelationManager::generateActivityId(const std::string& activity_name) {
    if (config_.activity_id_generator) {
        return config_.activity_id_generator(activity_name);
    }
    
    std::stringstream ss;
    if (!process_id_.empty()) {
        ss << process_id_ << "-";
    }
    ss << config_.activity_id_prefix << "-" << activity_name << "-" << generateDefaultUuid().substr(0, 8);
    return ss.str();
}

void CorrelationManager::loadFromEnvironment() {
    const char* pipeline_env = std::getenv(config_.env_var_pipeline.c_str());
    if (pipeline_env) {
        pipeline_id_ = pipeline_env;
    }
    
    const char* process_env = std::getenv(config_.env_var_process.c_str());
    if (process_env) {
        process_id_ = process_env;
    }
}

void CorrelationManager::saveToEnvironment() const {
    if (!config_.propagate_to_environment) {
        return;
    }
    
    if (!pipeline_id_.empty()) {
        setenv(config_.env_var_pipeline.c_str(), pipeline_id_.c_str(), 1);
    }
    
    if (!process_id_.empty()) {
        setenv(config_.env_var_process.c_str(), process_id_.c_str(), 1);
    }
}

std::string CorrelationManager::getFullCorrelationId() const {
    std::stringstream ss;
    
    if (!pipeline_id_.empty()) {
        ss << "pipeline:" << pipeline_id_;
    }
    
    if (!process_id_.empty()) {
        if (ss.tellp() > 0) ss << "|";
        ss << "process:" << process_id_;
    }
    
    if (!thread_activity_id_.empty()) {
        if (ss.tellp() > 0) ss << "|";
        ss << "activity:" << thread_activity_id_;
    }
    
    return ss.str();
}

std::unordered_map<std::string, std::string> CorrelationManager::getCorrelationContext() const {
    std::unordered_map<std::string, std::string> context;
    
    if (!pipeline_id_.empty()) {
        context["pipeline_id"] = pipeline_id_;
    }
    
    if (!process_id_.empty()) {
        context["process_id"] = process_id_;
    }
    
    if (!thread_activity_id_.empty()) {
        context["activity_id"] = thread_activity_id_;
    }
    
    return context;
}

void CorrelationManager::reset() {
    pipeline_id_.clear();
    process_id_.clear();
    thread_activity_id_.clear();
}

const CorrelationConfig& CorrelationManager::getConfig() const {
    return config_;
}

// ActivityScope implementation
ActivityScope::ActivityScope(const std::string& activity_name) {
    auto& manager = CorrelationManager::getInstance();
    previous_id_ = manager.getActivityId();
    activity_id_ = manager.generateActivityId(activity_name);
    manager.setActivityId(activity_id_);
}

ActivityScope::ActivityScope(const std::string& activity_name, 
                           const std::unordered_map<std::string, std::string>& context)
    : ActivityScope(activity_name) {
    context_ = context;
}

ActivityScope::~ActivityScope() {
    auto& manager = CorrelationManager::getInstance();
    if (previous_id_.empty()) {
        manager.clearActivityId();
    } else {
        manager.setActivityId(previous_id_);
    }
}

std::string ActivityScope::getActivityId() const {
    return activity_id_;
}

void ActivityScope::addContext(const std::string& key, const std::string& value) {
    context_[key] = value;
}

void ActivityScope::addContext(const std::unordered_map<std::string, std::string>& context) {
    for (const auto& [key, value] : context) {
        context_[key] = value;
    }
}

// ProcessScope implementation
ProcessScope::ProcessScope(const std::string& process_type) 
    : created_new_pipeline_(false) {
    auto& manager = CorrelationManager::getInstance();
    
    // Try to load existing correlation from environment
    manager.loadFromEnvironment();
    
    // If no pipeline ID exists and auto-generation is enabled, create a new one
    if (manager.getPipelineId().empty() && manager.getConfig().auto_generate_pipeline) {
        manager.setPipelineId(manager.generatePipelineId());
        created_new_pipeline_ = true;
    }
    
    // Generate process ID for this process type
    if (manager.getConfig().auto_generate_process) {
        process_id_ = manager.generateProcessId(process_type);
        manager.setProcessId(process_id_);
    }
    
    // Save to environment for child processes
    manager.saveToEnvironment();
}

ProcessScope::ProcessScope(const std::string& process_type, 
                         const std::unordered_map<std::string, std::string>& context)
    : ProcessScope(process_type) {
    context_ = context;
}

ProcessScope::~ProcessScope() {
    // Clean up environment variables if we created the pipeline
    if (created_new_pipeline_) {
        auto& manager = CorrelationManager::getInstance();
        unsetenv(manager.getConfig().env_var_pipeline.c_str());
        unsetenv(manager.getConfig().env_var_process.c_str());
    }
}

std::string ProcessScope::getProcessId() const {
    return process_id_;
}

} // namespace logservices
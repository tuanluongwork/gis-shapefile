#include "pxpoint_correlation.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace pxpoint {

thread_local std::string PxPointCorrelationManager::thread_activity_id_;

PxPointCorrelationManager& PxPointCorrelationManager::getInstance() {
    static PxPointCorrelationManager instance;
    return instance;
}

void PxPointCorrelationManager::setPipelineId(const std::string& pipeline_id) {
    pipeline_id_ = pipeline_id;
}

std::string PxPointCorrelationManager::getPipelineId() const {
    return pipeline_id_;
}

void PxPointCorrelationManager::setProcessId(const std::string& process_id) {
    process_id_ = process_id;
}

std::string PxPointCorrelationManager::getProcessId() const {
    return process_id_;
}

void PxPointCorrelationManager::setActivityId(const std::string& activity_id) {
    thread_activity_id_ = activity_id;
}

std::string PxPointCorrelationManager::getActivityId() const {
    return thread_activity_id_;
}

void PxPointCorrelationManager::clearActivityId() {
    thread_activity_id_.clear();
}

std::string PxPointCorrelationManager::generateUuid() {
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

std::string PxPointCorrelationManager::generatePipelineId() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << "pxp-" << time_t << "-" << generateUuid().substr(0, 8);
    return ss.str();
}

std::string PxPointCorrelationManager::generateProcessId(const std::string& process_type) {
    if (pipeline_id_.empty()) {
        pipeline_id_ = generatePipelineId();
    }
    
    std::stringstream ss;
    ss << pipeline_id_ << "-" << process_type << "-" << generateUuid().substr(0, 8);
    return ss.str();
}

std::string PxPointCorrelationManager::generateActivityId(const std::string& activity_name) {
    if (process_id_.empty()) {
        return activity_name + "-" + generateUuid().substr(0, 8);
    }
    
    std::stringstream ss;
    ss << process_id_ << "-" << activity_name << "-" << generateUuid().substr(0, 8);
    return ss.str();
}

void PxPointCorrelationManager::loadFromEnvironment() {
    const char* pipeline_env = std::getenv("PXPOINT_PIPELINE_ID");
    if (pipeline_env) {
        pipeline_id_ = pipeline_env;
    }
    
    const char* process_env = std::getenv("PXPOINT_PROCESS_ID");
    if (process_env) {
        process_id_ = process_env;
    }
}

void PxPointCorrelationManager::saveToEnvironment() {
    if (!pipeline_id_.empty()) {
        setenv("PXPOINT_PIPELINE_ID", pipeline_id_.c_str(), 1);
    }
    
    if (!process_id_.empty()) {
        setenv("PXPOINT_PROCESS_ID", process_id_.c_str(), 1);
    }
}

std::string PxPointCorrelationManager::getFullCorrelationId() const {
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

// ActivityCorrelationScope implementation
ActivityCorrelationScope::ActivityCorrelationScope(const std::string& activity_name) {
    auto& manager = PxPointCorrelationManager::getInstance();
    previous_id_ = manager.getActivityId();
    std::string new_activity_id = manager.generateActivityId(activity_name);
    manager.setActivityId(new_activity_id);
}

ActivityCorrelationScope::~ActivityCorrelationScope() {
    auto& manager = PxPointCorrelationManager::getInstance();
    if (previous_id_.empty()) {
        manager.clearActivityId();
    } else {
        manager.setActivityId(previous_id_);
    }
}

// ProcessCorrelationScope implementation
ProcessCorrelationScope::ProcessCorrelationScope(const std::string& process_type) 
    : created_new_pipeline_(false) {
    auto& manager = PxPointCorrelationManager::getInstance();
    
    // Try to load existing correlation from environment
    manager.loadFromEnvironment();
    
    // If no pipeline ID exists, create a new one
    if (manager.getPipelineId().empty()) {
        manager.setPipelineId(manager.generatePipelineId());
        created_new_pipeline_ = true;
    }
    
    // Generate process ID for this process type
    std::string process_id = manager.generateProcessId(process_type);
    manager.setProcessId(process_id);
    
    // Save to environment for child processes
    manager.saveToEnvironment();
}

ProcessCorrelationScope::~ProcessCorrelationScope() {
    // Clean up if we created the pipeline
    if (created_new_pipeline_) {
        unsetenv("PXPOINT_PIPELINE_ID");
        unsetenv("PXPOINT_PROCESS_ID");
    }
}

} // namespace pxpoint
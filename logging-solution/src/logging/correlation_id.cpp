#include "gis/correlation_id.h"
#include <random>
#include <sstream>
#include <iomanip>

namespace gis {

thread_local std::string CorrelationIdManager::thread_correlation_id_;

CorrelationIdManager& CorrelationIdManager::getInstance() {
    static CorrelationIdManager instance;
    return instance;
}

void CorrelationIdManager::setCorrelationId(const std::string& correlation_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    thread_correlation_id_ = correlation_id;
}

std::string CorrelationIdManager::getCorrelationId() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (thread_correlation_id_.empty()) {
        thread_correlation_id_ = const_cast<CorrelationIdManager*>(this)->generateCorrelationId();
    }
    return thread_correlation_id_;
}

void CorrelationIdManager::clearCorrelationId() {
    std::lock_guard<std::mutex> lock(mutex_);
    thread_correlation_id_.clear();
}

std::string CorrelationIdManager::generateCorrelationId() {
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

CorrelationIdScope::CorrelationIdScope(const std::string& correlation_id) {
    auto& manager = CorrelationIdManager::getInstance();
    previous_id_ = manager.getCorrelationId();
    manager.setCorrelationId(correlation_id);
}

CorrelationIdScope::~CorrelationIdScope() {
    auto& manager = CorrelationIdManager::getInstance();
    if (previous_id_.empty()) {
        manager.clearCorrelationId();
    } else {
        manager.setCorrelationId(previous_id_);
    }
}

} // namespace gis
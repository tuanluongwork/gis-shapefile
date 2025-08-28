#pragma once

#include <string>
#include <thread>
#include <unordered_map>
#include <mutex>

namespace gis {

class CorrelationIdManager {
public:
    static CorrelationIdManager& getInstance();
    
    void setCorrelationId(const std::string& correlation_id);
    std::string getCorrelationId() const;
    void clearCorrelationId();
    
    std::string generateCorrelationId();

private:
    CorrelationIdManager() = default;
    mutable std::mutex mutex_;
    thread_local static std::string thread_correlation_id_;
};

class CorrelationIdScope {
public:
    explicit CorrelationIdScope(const std::string& correlation_id);
    ~CorrelationIdScope();
    
private:
    std::string previous_id_;
};

} // namespace gis
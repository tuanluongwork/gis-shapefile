#pragma once

#include <string>
#include <thread>

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
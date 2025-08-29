# Enterprise Logging Solution for GIS Geocoding API

This directory contains the complete Phase 1: Foundation implementation of the enterprise logging architecture as specified in `../logging-implementation.md`.

## Directory Structure

```
plogger/
├── README.md                     # This file
├── README-Logging.md             # Detailed implementation guide
├── docker-compose.yml            # ELK stack orchestration
├── deploy-logging.sh             # Deployment script
├── include/gis/                  # C++ logging headers
│   ├── logger.h                  # Main logging interface
│   └── correlation_id.h          # Correlation ID management
├── src/logging/                  # C++ logging implementation
│   ├── logger.cpp                # Logger implementation
│   └── correlation_id.cpp        # Correlation ID implementation
├── config/                       # Application logging configuration
│   └── logging.yaml              # Logging settings
├── elk-config/                   # ELK stack configuration
│   ├── elasticsearch/            # Elasticsearch settings
│   ├── logstash/                 # Log processing pipeline
│   ├── kibana/                   # Visualization platform
│   └── filebeat/                 # Log shipping agent
└── kibana-dashboards/           # Pre-built monitoring dashboards
    └── gis-monitoring-dashboard.json
```

## Quick Start

### 1. Deploy the Logging Infrastructure

```bash
cd plogger
chmod +x deploy-logging.sh
./deploy-logging.sh
```

### 2. Integration with Existing Application

To integrate this logging solution with your GIS application:

1. **Copy headers to your include path**:
   ```bash
   cp -r plogger/include/gis/* /path/to/your/project/include/gis/
   ```

2. **Copy source files to your source path**:
   ```bash
   cp -r plogger/src/logging /path/to/your/project/src/
   ```

3. **Update your CMakeLists.txt**:
   ```cmake
   # Add spdlog dependency
   find_package(spdlog QUIET)
   if(NOT spdlog_FOUND)
       include(FetchContent)
       FetchContent_Declare(
           spdlog
           GIT_REPOSITORY https://github.com/gabime/spdlog.git
           GIT_TAG v1.12.0
       )
       FetchContent_MakeAvailable(spdlog)
   endif()
   
   # Add logging sources to your target
   target_sources(your-target PRIVATE
       src/logging/logger.cpp
       src/logging/correlation_id.cpp
   )
   
   # Link spdlog
   target_link_libraries(your-target spdlog::spdlog)
   ```

4. **Initialize logging in your application**:
   ```cpp
   #include "gis/logger.h"
   #include "gis/correlation_id.h"
   
   int main() {
       // Initialize logging
       gis::Logger::getInstance().initialize("info", "logs/your-app.log");
       
       // Your application code here
       LOG_INFO("Main", "Application started", {{"version", "1.0.0"}});
       
       return 0;
   }
   ```

### 3. Using the Logging System

```cpp
// Basic logging
LOG_INFO("ComponentName", "Operation completed successfully");

// Logging with context
LOG_INFO("GeocodingAPI", "Processing geocoding request", 
         {{"address", "123 Main St"}, {"user_id", "12345"}});

// Logging with performance metrics
LOG_INFO("DatabaseQuery", "Query executed", 
         {{"query_type", "SELECT"}, {"table", "addresses"}},
         {{"execution_time_ms", 150.5}, {"rows_returned", 42.0}});

// Correlation ID scoping
{
    gis::CorrelationIdScope scope("request-123-456");
    // All logs in this scope will include the correlation ID
    LOG_INFO("RequestHandler", "Processing user request");
}
```

## Features Implemented

### ✅ Phase 1: Foundation Complete

1. **Standardized Logging Framework**
   - spdlog integration with JSON output
   - Thread-safe multi-sink architecture
   - Configurable log levels and rotation

2. **Correlation ID System**
   - Thread-local correlation tracking
   - UUID v4 generation
   - RAII-based scope management

3. **Structured Logging Patterns**
   - JSON format with required fields
   - Context and performance metadata
   - Automatic field enrichment

4. **ELK Stack Infrastructure**
   - Elasticsearch for storage and indexing
   - Logstash for processing and enrichment
   - Kibana for visualization and monitoring
   - Filebeat for reliable log shipping

5. **Basic Monitoring Dashboards**
   - System overview dashboard
   - Performance monitoring
   - Error tracking and alerting

## Access Points

Once deployed, the logging infrastructure provides:

- **Kibana Dashboard**: http://localhost:5601
- **Elasticsearch API**: http://localhost:9200
- **Logstash API**: http://localhost:9600

## Configuration

### Logging Levels
- `FATAL`: System crashes, unrecoverable errors
- `ERROR`: Processing failures, exceptions requiring attention  
- `WARN`: Recoverable issues, performance degradation
- `INFO`: Normal operations, milestone events
- `DEBUG`: Detailed debugging information

### Performance Monitoring
- Automatic flagging of operations > 1000ms
- Memory usage tracking
- Request/response time metrics
- Database query performance

## Troubleshooting

### Common Issues

1. **Docker containers won't start**:
   ```bash
   docker-compose down
   docker system prune -f
   docker-compose up -d
   ```

2. **Logs not appearing in Kibana**:
   - Check Filebeat configuration
   - Verify log file permissions
   - Ensure Logstash pipeline is running

3. **Build integration issues**:
   - Verify spdlog is properly linked
   - Check include paths
   - Ensure C++17 compiler support

### Useful Commands

```bash
# View service status
docker-compose ps

# View service logs
docker-compose logs -f elasticsearch
docker-compose logs -f logstash
docker-compose logs -f kibana

# Test Elasticsearch
curl http://localhost:9200/_cluster/health

# Test Kibana
curl http://localhost:5601/api/status

# Restart services
docker-compose restart

# Stop services
docker-compose down
```

## Next Steps

This Phase 1 implementation provides the foundation for Phase 2 enhancements:
- Real-time Slack alerting
- Advanced performance monitoring  
- PII masking implementation
- Automated retention policies
- ML-based anomaly detection

For detailed implementation information, see `README-Logging.md`.
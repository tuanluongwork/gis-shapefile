# Logging Solution for GIS Geocoding API

This directory contains the core logging implementation for structured logging as specified in `../logging-implementation.md`.

## Directory Structure

```
log-services/
├── README.md                     # This file
├── README-Logging.md             # Detailed implementation guide
├── include/                      # C++ logging headers
│   ├── logger.h                  # Main logging interface
│   └── correlation_id.h          # Correlation ID management
├── src/                          # C++ logging implementation
│   ├── logger.cpp                # Logger implementation
│   └── correlation_id.cpp        # Correlation ID implementation
└── config/                       # Application logging configuration
    └── logging.yaml              # Logging settings
```

## Quick Start

### 1. Integration with Existing Application

To integrate this logging solution with your application:

1. **Copy headers to your include path**:
   ```bash
   cp -r log-services/include/* /path/to/your/project/include/
   ```

2. **Copy source files to your source path**:
   ```bash
   cp -r log-services/src/* /path/to/your/project/src/
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
       src/logger.cpp
       src/correlation_id.cpp
   )
   
   # Link spdlog
   target_link_libraries(your-target spdlog::spdlog)
   ```

4. **Initialize logging in your application**:
   ```cpp
   #include "logger.h"
   #include "correlation_id.h"
   
   int main() {
       // Initialize logging
       Logger::getInstance().initialize("info", "logs/your-app.log");
       
       // Your application code here
       LOG_INFO("Main", "Application started", {{"version", "1.0.0"}});
       
       return 0;
   }
   ```

### 2. Using the Logging System

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
    CorrelationIdScope scope("request-123-456");
    // All logs in this scope will include the correlation ID
    LOG_INFO("RequestHandler", "Processing user request");
}
```

## Features Implemented

### ✅ Core Logging Components

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

1. **Build integration issues**:
   - Verify spdlog is properly linked
   - Check include paths
   - Ensure C++17 compiler support

2. **Log file permissions**:
   - Ensure write permissions to log directory
   - Check disk space availability

3. **Configuration issues**:
   - Verify YAML configuration syntax
   - Check log level settings

## Next Steps

This implementation provides the foundation for future enhancements:
- Advanced performance monitoring  
- PII masking implementation
- Automated retention policies
- Integration with external log aggregation systems

For detailed implementation information, see `README-Logging.md`.
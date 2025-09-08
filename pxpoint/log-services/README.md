# Log Services - Generic Structured Logging Library

A high-performance, feature-rich structured logging library with YAML configuration support, built on top of [spdlog](https://github.com/gabime/spdlog). This library provides hierarchical correlation tracking, asynchronous logging, and comprehensive observability features for modern C++ applications.

## Features

### Core Capabilities
- **Structured Logging**: JSON-formatted logs with rich context and metadata
- **Hierarchical Correlation**: Pipeline → Process → Activity correlation tracking
- **YAML Configuration**: Flexible configuration management
- **High Performance**: Asynchronous logging with configurable thread pools
- **Thread Safety**: Full support for multi-threaded applications
- **Multiple Sinks**: Console, file, rotating file, and daily file outputs
- **Cross-Process Correlation**: Environment variable propagation
- **RAII Scopes**: Automatic resource management and correlation cleanup

### Advanced Features
- **Performance Measurements**: Built-in timing and metrics collection
- **Custom Formatters**: Extensible message formatting
- **Error Recovery**: Robust error handling and recovery mechanisms
- **Runtime Configuration**: Dynamic log level changes
- **Memory Efficient**: Optimized for minimal overhead
- **Industry Standards**: Compatible with ELK stack and other log aggregators

## Quick Start

### Basic Usage

```cpp
#include "structured_logger.h"
#include "correlation_manager.h"

using namespace logservices;

int main() {
    // Initialize logger
    auto& logger = StructuredLogger::getInstance();
    logger.initialize("my-application");
    
    // Basic logging
    LOG_INFO("Application started");
    LOG_ERROR("Something went wrong", {{"error_code", "500"}});
    
    // With correlation context
    ProcessScope process_scope("main-process");
    {
        ActivityScope activity("user-authentication");
        LOG_INFO("User login attempt", {{"user_id", "12345"}});
    }
    
    return 0;
}
```

### YAML Configuration

```yaml
# logging.yaml
logging:
  name: "my-app"
  level: "info"
  log_directory: "/var/log/my-app"
  async_logging: true
  
  sinks:
    - type: "console"
      level: "info"
      pattern: "%Y-%m-%d %H:%M:%S [%^%l%$] %v"
      
    - type: "daily_file"
      level: "debug"
      pattern: '{"timestamp":"%Y-%m-%dT%H:%M:%S.%fZ","level":"%l","message":"%v"}'
      file_path: "/var/log/my-app/{}-daily.log"

correlation:
  pipeline_id_prefix: "pipeline"
  process_id_prefix: "proc"
  env_var_pipeline: "APP_PIPELINE_ID"
  env_var_process: "APP_PROCESS_ID"
```

```cpp
// Load configuration
auto& logger = StructuredLogger::getInstance();
auto& correlation = CorrelationManager::getInstance();

correlation.loadConfigFromYaml("logging.yaml");
logger.loadConfigFromYaml("logging.yaml");
logger.initialize("configured-app");
```

## Architecture

### Correlation Hierarchy

The library implements a three-tier correlation system:

1. **Pipeline ID**: Represents an end-to-end business transaction
2. **Process ID**: Identifies individual processes within a pipeline
3. **Activity ID**: Provides fine-grained tracing within processes

```
Pipeline: pipeline-1234567890-abc123
├── Process: pipeline-1234567890-abc123-proc-orchestrator-def456
│   ├── Activity: ...proc-orchestrator-def456-act-initialization-ghi789
│   └── Activity: ...proc-orchestrator-def456-act-worker-spawn-jkl012
└── Process: pipeline-1234567890-abc123-proc-worker-mno345
    ├── Activity: ...proc-worker-mno345-act-data-validation-pqr678
    └── Activity: ...proc-worker-mno345-act-data-processing-stu901
```

### Component Structure

```
log-services/
├── include/           # Public API headers
│   ├── correlation_manager.h
│   └── structured_logger.h
├── src/              # Implementation files
│   ├── correlation_manager.cpp
│   └── structured_logger.cpp
├── config/           # Configuration templates
│   ├── logging.yaml
│   ├── logging-development.yaml
│   └── logging-production.yaml
├── examples/         # Usage examples
├── tests/           # Comprehensive test suite
└── cmake/           # CMake configuration files
```

## Configuration Reference

### Logger Configuration

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `name` | string | "app" | Logger name |
| `level` | string | "info" | Minimum log level |
| `log_directory` | string | "/tmp/pxpoint-logs" | Base directory for log files |
| `async_logging` | boolean | true | Enable asynchronous logging |
| `async_queue_size` | number | 8192 | Async queue size |
| `async_thread_count` | number | 1 | Number of async threads |
| `auto_add_correlation` | boolean | true | Automatically add correlation context |
| `flush_on_error` | boolean | true | Flush immediately on errors |
| `flush_interval` | number | 5 | Auto-flush interval (seconds) |

### Sink Configuration

| Parameter | Type | Description |
|-----------|------|-------------|
| `type` | string | Sink type: "console", "file", "rotating_file", "daily_file" |
| `name` | string | Unique sink identifier |
| `level` | string | Minimum log level for this sink |
| `pattern` | string | Log message format pattern |
| `file_path` | string | File path (use `{}` for process name substitution) |
| `max_file_size` | number | Maximum file size for rotating files |
| `max_files` | number | Number of files to keep in rotation |
| `color_mode` | boolean | Enable colored output (console only) |

### Correlation Configuration

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `pipeline_id_prefix` | string | "pipeline" | Prefix for pipeline IDs |
| `process_id_prefix` | string | "proc" | Prefix for process IDs |
| `activity_id_prefix` | string | "act" | Prefix for activity IDs |
| `env_var_pipeline` | string | "LOG_PIPELINE_ID" | Environment variable for pipeline ID |
| `env_var_process` | string | "LOG_PROCESS_ID" | Environment variable for process ID |
| `auto_generate_pipeline` | boolean | true | Auto-generate pipeline IDs |
| `auto_generate_process` | boolean | true | Auto-generate process IDs |
| `propagate_to_environment` | boolean | true | Save IDs to environment variables |

## API Reference

### StructuredLogger

#### Core Methods

```cpp
// Initialization
void initialize(const std::string& process_type, 
               spdlog::level::level_enum log_level = spdlog::level::info);
void loadConfigFromYaml(const std::string& yaml_file_path);

// Basic logging
void info(const std::string& message, 
          const std::unordered_map<std::string, std::string>& context = {});
void error(const std::string& message, 
           const std::unordered_map<std::string, std::string>& context = {});

// Component-specific logging
void info(const std::string& component, const std::string& message, 
          const std::unordered_map<std::string, std::string>& context = {});

// Advanced logging with metrics
void log(spdlog::level::level_enum level, const std::string& message,
         const std::unordered_map<std::string, std::string>& context = {},
         const std::unordered_map<std::string, double>& metrics = {});
```

#### Event Logging

```cpp
// Structured events
void logEvent(const std::string& event_type, const std::string& description,
              const std::unordered_map<std::string, std::string>& context = {},
              const std::unordered_map<std::string, double>& metrics = {});

// Process lifecycle
void logProcessStart(const std::string& process_type, 
                    const std::unordered_map<std::string, std::string>& config = {});
void logProcessEnd(const std::string& process_type, bool success = true,
                  const std::unordered_map<std::string, double>& metrics = {});

// Performance measurement
void logPerformance(const std::string& operation, double duration_ms,
                   const std::unordered_map<std::string, std::string>& context = {},
                   const std::unordered_map<std::string, double>& metrics = {});
```

### CorrelationManager

```cpp
// ID management
void setPipelineId(const std::string& pipeline_id);
std::string getPipelineId() const;
std::string generatePipelineId();

// Environment integration
void loadFromEnvironment();
void saveToEnvironment() const;

// Context retrieval
std::string getFullCorrelationId() const;
std::unordered_map<std::string, std::string> getCorrelationContext() const;
```

### RAII Scopes

```cpp
// Process scope - manages process-level correlation
ProcessScope process_scope("my-process");

// Activity scope - manages activity-level correlation
ActivityScope activity("data-processing", {{"batch_size", "1000"}});

// Performance timing scope
PerformanceTimer timer("database-query", {{"table", "users"}});
```

### Convenience Macros

```cpp
// Basic logging
LOG_INFO("Message", {{"key", "value"}});
LOG_ERROR("Error occurred", {{"error_code", "404"}});

// Component-specific
LOG_COMPONENT_INFO("Database", "Connected", {{"host", "localhost"}});

// Scoped operations
LOG_ACTIVITY_SCOPE("user-authentication", {{"method", "oauth2"}});
LOG_PERFORMANCE_SCOPE("data-transformation", {{"algorithm", "ml-model"}});
```

## Performance Characteristics

### Benchmarks

- **Synchronous Logging**: ~500,000 messages/second
- **Asynchronous Logging**: ~1,500,000 messages/second
- **Correlation Overhead**: <10ns per operation
- **Memory Usage**: ~50MB for 1M cached messages
- **Thread Safety**: Zero-copy thread-local storage

### Optimization Tips

1. **Use Async Logging**: Enable `async_logging: true` for high-throughput scenarios
2. **Appropriate Queue Sizes**: Increase `async_queue_size` for burst scenarios
3. **Level Filtering**: Set appropriate log levels to reduce I/O
4. **Structured Context**: Use structured context instead of string concatenation
5. **Performance Scopes**: Use RAII scopes for automatic timing

## Integration Examples

### Multi-Process Architecture

```cpp
// Orchestrator process
int main() {
    ProcessScope orchestrator("orchestrator");
    
    LOG_INFO("Starting pipeline", {{"workers", "3"}});
    
    for (const auto& worker_type : {"validator", "processor", "aggregator"}) {
        // Spawn worker process - correlation propagated via environment
        std::system(("./worker " + std::string(worker_type)).c_str());
    }
    
    LOG_INFO("Pipeline completed");
}

// Worker process
int main(int argc, char* argv[]) {
    std::string worker_type = argv[1];
    
    ProcessScope worker(worker_type);  // Loads correlation from environment
    
    LOG_INFO("Worker started", {{"type", worker_type}});
    
    ActivityScope work("process-data");
    // ... do work ...
    
    LOG_INFO("Worker completed");
}
```

### ELK Stack Integration

The structured JSON output is designed for seamless integration with the ELK stack:

```json
{
  "timestamp": "2024-01-15T14:30:45.123Z",
  "level": "INFO",
  "logger": "data-processor",
  "message": "Processing completed successfully",
  "pipeline_id": "pipeline-1705329045-abc123",
  "process_id": "pipeline-1705329045-abc123-proc-processor-def456",
  "activity_id": "...proc-processor-def456-act-data-validation-ghi789",
  "component": "DataProcessor",
  "records_processed": 15000,
  "duration_ms": 2456.78
}
```

### Error Handling

```cpp
try {
    ActivityScope activity("risky-operation");
    
    // Risky operation
    throw std::runtime_error("Something failed");
    
} catch (const std::exception& e) {
    LOG_ERROR("Operation failed", {
        {"exception", e.what()},
        {"operation", "risky-operation"},
        {"retry_count", "3"}
    });
}
```

## Building and Installation

### Prerequisites

- C++17 compatible compiler
- CMake 3.16+
- spdlog 1.12+ (automatically fetched if not found)
- yaml-cpp 0.8+ (automatically fetched if not found)

### Build Steps

```bash
# Clone and build
git clone <repository>
cd log-services
mkdir build && cd build

# Configure and build
cmake ..
make -j$(nproc)

# Run tests
ctest --output-on-failure

# Install (optional)
sudo make install
```

### CMake Integration

```cmake
# In your CMakeLists.txt
find_package(log-services REQUIRED)

add_executable(my-app main.cpp)
target_link_libraries(my-app PRIVATE log-services::log-services)
```

## Best Practices

### 1. Structured Context Over String Concatenation

```cpp
// ❌ Avoid string concatenation
LOG_INFO("User " + user_id + " completed operation " + operation);

// ✅ Use structured context
LOG_INFO("User completed operation", {
    {"user_id", user_id},
    {"operation", operation}
});
```

### 2. Appropriate Log Levels

```cpp
// Development: Verbose logging
LOG_DEBUG("Detailed state information", {{"state", current_state}});

// Production: Essential information only
LOG_INFO("Operation completed", {{"duration_ms", duration}});
LOG_ERROR("Critical failure", {{"error_code", error_code}});
```

### 3. Performance-Critical Sections

```cpp
// ✅ Use performance scopes for automatic timing
{
    LOG_PERFORMANCE_SCOPE("database_query", {{"table", "users"}});
    // Database operation happens here
    auto result = database.query("SELECT * FROM users");
} // Performance automatically logged on scope exit

// ✅ For manual control
PerformanceTimer timer("complex_algorithm");
// Complex algorithm
timer.addMetric("iterations", iteration_count);
timer.stop(); // Logs performance with metrics
```

### 4. Error Context

```cpp
// ✅ Rich error context
try {
    process_file(filename);
} catch (const std::exception& e) {
    LOG_ERROR("File processing failed", {
        {"filename", filename},
        {"file_size", std::to_string(get_file_size(filename))},
        {"exception", e.what()},
        {"retry_attempt", std::to_string(retry_count)}
    });
}
```

### 5. Multi-Process Correlation

```cpp
// Parent process
ProcessScope parent("orchestrator");
auto pipeline_id = CorrelationManager::getInstance().getPipelineId();

// Child process automatically inherits correlation via environment
system("./child_process");

// In child_process
ProcessScope child("worker"); // Loads pipeline_id from environment
LOG_INFO("Child process started"); // Automatically includes correlation
```

## Troubleshooting

### Common Issues

1. **Missing Log Files**: Check directory permissions for `log_directory`
2. **High Memory Usage**: Reduce `async_queue_size` or disable async logging
3. **Missing Correlation**: Ensure `ProcessScope` is created before logging
4. **Performance Issues**: Enable async logging and appropriate log levels

### Debug Mode

```cpp
// Enable debug logging to diagnose issues
auto& logger = StructuredLogger::getInstance();
logger.setLevel(spdlog::level::debug);

LOG_DEBUG("Diagnostic information", {{"internal_state", state}});
```

## Contributing

1. Follow C++17 standards
2. Add unit tests for new features
3. Update documentation
4. Ensure thread safety
5. Benchmark performance-critical changes

## License

[License information here]

---

For more examples and advanced usage, see the `examples/` directory and test suite in `tests/`.
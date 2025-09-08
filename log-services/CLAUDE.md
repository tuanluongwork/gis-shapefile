# log-services: A Structured Logging Solution for C++

As a Principal Software Engineer, this document provides a detailed summary of the `log-services` logging solution, designed to bring robust, structured, and observable logging to the GIS Geocoding API and other C++ applications.

## 1. Overview

`log-services` is a modern C++ logging library built as a wrapper around the popular `spdlog` library. It is engineered to enforce a structured, JSON-based logging standard, which is critical for effective log management, parsing, and analysis in a microservices or complex application environment. The primary goal of `log-services` is to provide a centralized, easy-to-integrate, and feature-rich logging framework that promotes consistency and traceability across the entire system.

## 2. Core Features

The solution is built on a set of key features that address common challenges in application logging.

### a. Structured JSON Logging

All log entries are generated in a standardized JSON format. This ensures that logs are machine-readable and can be easily ingested by log aggregation platforms like ELK (Elasticsearch, Logstash, Kibana) or Splunk.

A typical log entry includes:
- `timestamp`: ISO 8601 formatted timestamp.
- `level`: Log severity (e.g., `INFO`, `WARN`, `ERROR`).
- `logger_name`: The component or module emitting the log.
- `message`: The main log message.
- `correlation_id`: A unique identifier for tracking a request or transaction.
- `context`: A JSON object for application-specific key-value pairs (e.g., user ID, request parameters).
- `performance`: A JSON object for performance metrics (e.g., `execution_time_ms`, `memory_usage_mb`).

### b. High-Performance Asynchronous Logging
To minimize the performance impact on the main application threads, `log-services` now operates in an asynchronous mode. Log messages are placed into a non-blocking queue and are written to the target sinks (file, console) by a dedicated background thread. This decouples the logging overhead from the application's critical path, resulting in lower latency and higher throughput.

### c. Correlation ID Management

To ensure end-to-end traceability, `log-services` implements a robust correlation ID system.
- **Thread-Local Storage**: Correlation IDs are stored in thread-local variables, ensuring that the correct ID is automatically associated with all logs generated within the same thread of execution.
- **RAII-based Scoping**: The `CorrelationIdScope` class uses the Resource Acquisition Is Initialization (RAII) pattern. A developer can instantiate a scope object at the beginning of a function or block, and the correlation ID is automatically managed and cleaned up when the scope is exited. This makes it simple and safe to manage the lifecycle of a correlation ID.

### d. Performance Monitoring

The library includes built-in support for capturing and logging performance metrics.
- **Automatic Flagging**: Operations exceeding a predefined threshold (e.g., 1000ms) can be automatically flagged as a `WARN` or `ERROR`, drawing immediate attention to performance bottlenecks.
- **Dedicated Metrics**: The `performance` field in the JSON output is reserved for numerical metrics, allowing for easy querying and visualization in monitoring dashboards.

### e. Flexible Configuration

`log-services` is designed to be highly configurable without requiring code changes.
- **YAML-based Configuration**: Logging behavior (level, output file, rotation) is defined in a `logging.yaml` file.
- **Multi-Sink Architecture**: It supports multiple log outputs simultaneously. For example, it can be configured to log to both a rotating file on disk and the standard console, with different log levels for each.
- **Log Rotation**: Out-of-the-box support for daily or size-based log file rotation.

## 3. Architecture and Implementation

The library is designed for simplicity and efficiency.

- **`Logger` Singleton**: A singleton `Logger` class provides a global access point for the logging functionality, ensuring a single, consistent logging configuration throughout the application.
- **Asynchronous Architecture**: The logger now uses `spdlog::async_logger`, which leverages a dedicated thread pool to process and write log messages. This prevents I/O operations from blocking application threads. A custom `async_hybrid_file_sink` is implemented to handle file rotation in this asynchronous context.
- **Header-Only and Source Files**: The library is provided as a set of headers (`logger.h`, `correlation_id.h`) and corresponding source files (`logger.cpp`, `correlation_id.cpp`), making it easy to integrate into any C++ project.
- **CMake Integration**: The project includes instructions for integrating with CMake using `FetchContent` for the `spdlog` dependency, simplifying the build process.
- **C++17**: The implementation leverages modern C++17 features for cleaner, more efficient, and safer code.

## 4. Usage and Integration

Integrating and using `log-services` is straightforward.

### a. Initialization

In the application's entry point (`main.cpp`), the logger is initialized once:
```cpp
#include "logger.h"

int main() {
    Logger::getInstance().initialize("info", "logs/gis-app.log");
    LOG_INFO("Application", "Server starting up", {{"version", "1.2.0"}});
    // ...
    return 0;
}
```

### b. Logging Macros

A set of intuitive macros (`LOG_INFO`, `LOG_WARN`, `LOG_ERROR`, etc.) are provided for easy use:

```cpp
// Simple log message
LOG_INFO("Database", "Connection established successfully.");

// Log with contextual data
LOG_DEBUG("HTTPRequest", "Received new request", {{"method", "GET"}, {"path", "/geocode"}});

// Log with performance metrics
LOG_INFO("GeocodingService", "Address processed",
         {{"address", "1600 Pennsylvania Ave NW"}},
         {{"processing_time_ms", 75.5}});
```

### c. Correlation ID Scope

```cpp
#include "correlation_id.h"

void handle_request(const std::string& requestId) {
    // All logs within this function will have the same correlation ID
    CorrelationIdScope scope(requestId);

    LOG_INFO("RequestHandler", "Processing started.");
    // ...
    LOG_INFO("RequestHandler", "Processing finished.");
}
```

### d. Shutdown
Because the logger operates asynchronously, it is crucial to shut it down gracefully before the application terminates. This ensures that all buffered logs are flushed to their destinations.

```cpp
#include "logger.h"

int main() {
    Logger::getInstance().initialize("info", "logs/gis-app.log");
    // ... application logic ...
    LOG_INFO("Application", "Server shutting down.");
    Logger::getInstance().shutdown(); // Important: Flush all logs before exit
    return 0;
}
```

## 5. Conclusion

The `log-services` solution provides a professional, enterprise-grade logging framework for C++ applications. By enforcing structured JSON logging and providing built-in support for correlation IDs and performance monitoring, it significantly enhances the observability and maintainability of the GIS Geocoding API. Its ease of integration and clear usage patterns make it an invaluable tool for developers, enabling them to produce consistent, context-rich logs with minimal effort. The addition of asynchronous logging further enhances its performance, making it suitable for high-throughput, low-latency systems. This directly translates to faster debugging, better performance analysis, and more reliable operations.

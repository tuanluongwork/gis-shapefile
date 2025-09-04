# GIS Geocoding API - Logging Implementation

## Overview

This document describes the core logging implementation for the GIS Geocoding API system, following the specifications in `logging-implementation.md`.

## What's Implemented

### 1. Standardized Logging Framework ✅

- **spdlog Integration**: High-performance C++ logging library with JSON output capability
- **Structured Logging**: All logs output in JSON format with standardized fields
- **Multi-sink Architecture**: Console and rotating file outputs
- **Thread-safe Operations**: Full thread safety for concurrent operations

### 2. Correlation ID System ✅

- **Thread-local Storage**: Each request gets a unique correlation ID
- **Automatic Propagation**: Correlation IDs automatically included in all log messages
- **UUID Generation**: Proper UUID v4 generation for unique identification
- **Scoped Management**: RAII-based correlation ID scope management

### 3. Structured Log Fields ✅

All logs include the required fields as specified:
- **Core Fields**: timestamp, level, logger_name, message
- **Context Fields**: correlation_id, fips, job_id, process_step, user_id (when applicable)
- **Performance Fields**: execution_time_ms, memory_usage_mb, response_time_ms
- **Error Fields**: error_code, stack_trace, error_context (when applicable)


## Quick Start

### Prerequisites

- CMake 3.15+
- C++17 compiler
- spdlog library (handled by CMake FetchContent)

### Integration

```bash
# 1. Build the application with logging
make build

# 2. Run the server
make run
```

### Usage

The logging system will automatically create log files in the configured directory and output structured JSON logs.

## Usage Examples

### Basic Logging Usage

When your application runs, it will generate structured logs like:
```json
{
  "timestamp": "2025-01-08T14:30:00.000Z",
  "level": "INFO",
  "logger": "GeocodingAPI",
  "message": "Processing request | correlation_id:a1b2c3d4-e5f6-4789-0123-456789abcdef path:/geocode query:address=1600+Amphitheatre+Parkway"
}
```

### Performance Monitoring

All operations include performance metrics:
```json
{
  "timestamp": "2025-01-08T14:30:01.500Z",
  "level": "INFO", 
  "logger": "GeocodingAPI",
  "message": "Geocoding successful | correlation_id:a1b2c3d4-e5f6-4789-0123-456789abcdef input_address:1600 Amphitheatre Parkway matched_address:1600 Amphitheatre Pkwy, Mountain View, CA confidence:0.95 geocode_time_ms:150.5"
}
```

## Architecture

### Log Flow
```
Application → spdlog → Log Files (JSON format) → Console/File Output
```

### File Structure
```
plogger/
├── include/
│   ├── logger.h              # Main logging interface
│   └── correlation_id.h      # Correlation ID management
├── src/
│   ├── logger.cpp            # Logger implementation
│   └── correlation_id.cpp    # Correlation ID implementation
└── config/
    └── logging.yaml          # Logging configuration
```

## Configuration Options

### Log Levels
- **DEBUG**: Detailed debugging information
- **INFO**: Normal operations and milestone events
- **WARN**: Recoverable issues and warnings
- **ERROR**: Processing failures and exceptions
- **FATAL**: System crashes and unrecoverable errors

### Output Configuration
- **Console Output**: Human-readable format for development
- **File Output**: Structured JSON format for production
- **Log Rotation**: Automatic file rotation based on size/time

### Performance Tracking
- Operations are logged with execution time
- Memory usage tracking available
- Error tracking with full context

## Troubleshooting

### Common Issues

1. **Build failures**: Ensure spdlog is properly installed via CMake and C++17 support is available
2. **Log file creation failures**: Check directory permissions and disk space
3. **Configuration errors**: Verify YAML syntax in logging.yaml

### Verification

```bash
# Check if logs are being created
ls -la logs/

# View recent log entries
tail -f logs/application.log

# Validate JSON format
cat logs/application.log | jq '.'
```

## Performance Considerations

- Log rotation: Files rotate at 5MB with 10 file retention by default
- Asynchronous logging: Minimal performance impact on main application
- JSON formatting: Structured output suitable for log analysis tools
- Memory usage: Efficient buffering to minimize overhead

This implementation provides a solid foundation for structured logging in the GIS Geocoding API system.
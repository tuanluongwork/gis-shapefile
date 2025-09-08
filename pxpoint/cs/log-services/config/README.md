# Configuration Files

This directory contains default configuration files for the LogServices library.

## Files

### `correlation.json`
Configuration for hierarchical correlation behavior:
- **Pipeline/Process/Activity prefixes**: Customize correlation ID formats
- **Environment variables**: Configure which env vars are used for cross-process propagation
- **Auto-generation**: Control automatic ID generation behavior
- **Custom generators**: Can be configured programmatically (not via JSON)

### `logging.json`
Configuration for structured logging:
- **Log levels**: Set minimum logging levels globally and per-sink
- **Async logging**: Configure queue sizes and thread counts
- **Sinks**: Define multiple output destinations (console, files, rotating files)
- **File paths**: Support variable substitution like `{process_type}`, `{date}`
- **Auto-correlation**: Enable/disable automatic correlation context enrichment

## Usage

### Load Both Configurations
```csharp
// Recommended: Load both config files at startup
CorrelationManager.Instance.LoadConfigFromJson("config/correlation.json");
StructuredLogger.Instance.LoadConfigFromJson("config/logging.json");
```

### Override Specific Settings
```csharp
// Load base config, then override specific settings
StructuredLogger.Instance.LoadConfigFromJson("config/logging.json");
StructuredLogger.Instance.SetLevel(LogLevel.Debug);  // Override log level
```

### Environment-Specific Configs
```csharp
// Load different configs based on environment
var env = Environment.GetEnvironmentVariable("ENVIRONMENT") ?? "development";
CorrelationManager.Instance.LoadConfigFromJson($"config/correlation-{env}.json");
StructuredLogger.Instance.LoadConfigFromJson($"config/logging-{env}.json");
```

## Configuration Schema

### Correlation Configuration Properties
```json
{
  "pipelineIdPrefix": "string",      // Prefix for pipeline IDs
  "processIdPrefix": "string",       // Prefix for process IDs  
  "activityIdPrefix": "string",      // Prefix for activity IDs
  "envVarPipeline": "string",        // Environment variable for pipeline ID
  "envVarProcess": "string",         // Environment variable for process ID
  "autoGeneratePipeline": boolean,   // Auto-generate pipeline IDs
  "autoGenerateProcess": boolean,    // Auto-generate process IDs
  "propagateToEnvironment": boolean  // Set env vars for child processes
}
```

### Logger Configuration Properties
```json
{
  "name": "string",                  // Logger instance name
  "level": "Debug|Info|Warning|Error|Critical",  // Minimum log level
  "defaultPattern": "string",        // Text log format pattern
  "jsonPattern": "json",             // JSON log format (always "json")
  "asyncLogging": boolean,           // Enable async background logging
  "asyncQueueSize": number,          // Size of async log queue
  "asyncThreadCount": number,        // Number of background threads
  "asyncOverflowPolicy": "string",   // Queue overflow behavior
  "logDirectory": "string",          // Base directory for log files
  "autoAddCorrelation": boolean,     // Auto-add correlation context
  "flushOnError": boolean,           // Flush immediately on errors
  "flushIntervalSeconds": number,    // Periodic flush interval
  "sinks": [                         // Output destinations
    {
      "type": "Console|File|RotatingFile|DailyFile",
      "name": "string",              // Unique sink name
      "level": "Debug|Info|Warning|Error|Critical",  // Sink-specific level
      "pattern": "string",           // Sink-specific pattern
      "filePath": "string",          // File path (for file sinks)
      "maxFileSize": number,         // Max file size in bytes
      "maxFiles": number,            // Max number of rotated files
      "rotationHour": number,        // Daily rotation hour (0-23)
      "rotationMinute": number,      // Daily rotation minute (0-59)
      "colorMode": boolean           // Console color output
    }
  ]
}
```

## File Path Variables

The following variables can be used in file paths:
- `{process_type}`: The process type name passed to `Initialize()`
- `{date}`: Current date in YYYY-MM-DD format
- `{timestamp}`: Unix timestamp
- `{log_directory}`: The configured log directory

Example: `/var/log/myapp/{process_type}-{date}.log`

## Best Practices

1. **Version Control**: Keep default configs in version control
2. **Environment Overrides**: Use environment-specific config files for different deployments
3. **Validation**: The library will validate configuration on load and throw exceptions for invalid configs
4. **Hot Reload**: Configuration is loaded once at startup; restart required for changes
5. **Security**: Avoid sensitive information in config files; use environment variables for secrets
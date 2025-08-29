# GIS Geocoding API - Phase 1 Logging Implementation

## Overview

This document describes the Phase 1: Foundation implementation of the enterprise logging architecture for the GIS Geocoding API system, following the specifications in `logging-implementation.md`.

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

### 4. ELK Stack Infrastructure ✅

- **Elasticsearch 8.11.0**: Primary log storage with optimized indexing
- **Logstash 8.11.0**: Data transformation and enrichment pipeline
- **Kibana 8.11.0**: Visualization and monitoring platform
- **Filebeat 8.11.0**: Lightweight log shipper

### 5. Log Processing Pipeline ✅

- **Structured Parsing**: Automatic parsing of JSON log entries
- **Field Enrichment**: Addition of service metadata and performance flags
- **Index Management**: Time-based indices with proper field mappings
- **Performance Alerting**: Automatic flagging of slow operations

## Quick Start

### Prerequisites

- Docker and Docker Compose
- CMake 3.15+
- C++17 compiler
- Make

### Deployment

```bash
# 1. Deploy logging infrastructure
make deploy-logging

# 2. Build the application
make build

# 3. Run the server
make run
```

### Verify Installation

```bash
# Check service status
make status

# Test logging infrastructure
make test-logging

# View logs in real-time
make logs
```

### Access Points

- **Elasticsearch**: http://localhost:9200
- **Kibana**: http://localhost:5601
- **Logstash API**: http://localhost:9600
- **GIS API**: http://localhost:8080

## Usage Examples

### Basic API Request with Logging

```bash
curl "http://localhost:8080/geocode?address=1600+Amphitheatre+Parkway"
```

This will generate structured logs like:
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
Application → spdlog → Log File → Filebeat → Logstash → Elasticsearch → Kibana
```

### File Structure
```
gis-shapefile-main/
├── include/gis/
│   ├── logger.h              # Main logging interface
│   └── correlation_id.h      # Correlation ID management
├── src/logging/
│   ├── logger.cpp            # Logger implementation
│   └── correlation_id.cpp    # Correlation ID implementation
├── config/
│   └── logging.yaml          # Logging configuration
└── elk-infrastructure/       # ELK stack deployment
    ├── docker-compose.yml    # ELK stack orchestration  
    ├── deploy-logging.sh     # Deployment script
    ├── elk-config/
    │   ├── elasticsearch/    # Elasticsearch configuration
    │   ├── logstash/         # Logstash pipeline configuration
    │   ├── kibana/           # Kibana configuration
    │   └── filebeat/         # Filebeat configuration
    └── kibana-dashboards/    # Pre-built monitoring dashboards
```

## Monitoring and Alerts

### Performance Metrics Tracked
- Request response times
- Geocoding operation times
- Memory usage patterns
- Error rates and types

### Automatic Alerting
- Operations taking > 1000ms are tagged as `performance_alert`
- Failed geocoding attempts are logged with context
- System health checks are logged every 30 seconds

## Next Steps (Phase 2)

The foundation is now in place. Phase 2 will include:
- Real-time Slack alerting integration
- Advanced performance monitoring
- PII masking implementation
- Retention policy automation

## Troubleshooting

### Common Issues

1. **Elasticsearch won't start**: Check available memory (requires 512MB minimum)
2. **Logs not appearing**: Verify log file permissions and Filebeat configuration
3. **Build failures**: Ensure spdlog is properly installed via CMake

### Commands

```bash
# View service logs
docker-compose logs elasticsearch
docker-compose logs logstash
docker-compose logs kibana

# Restart services
docker-compose restart

# Check Elasticsearch health
curl http://localhost:9200/_cluster/health

# Test log ingestion
echo '{"test":"message"}' | curl -X POST http://localhost:5000 -d @-
```

## Performance Considerations

- Log rotation: Files rotate at 5MB with 10 file retention
- Elasticsearch: Single node development setup (scale for production)
- Logstash: Configured for 2 workers with batch processing
- Network overhead: Minimal with local Docker networking

This Phase 1 implementation provides the solid foundation required for enterprise-grade logging and monitoring of the GIS Geocoding API system.
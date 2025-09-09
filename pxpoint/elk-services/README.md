# Generic ELK Stack for Log Analysis

A flexible and configurable ELK (Elasticsearch, Logstash, Kibana) stack with Filebeat for analyzing logs from any application. Decoupled from specific applications, this stack can be configured to work with various log formats and sources.

## 🏗️ Architecture

- **Elasticsearch**: Search and analytics engine for storing logs
- **Logstash**: Data processing pipeline for parsing and transforming logs
- **Kibana**: Visualization and management interface
- **Filebeat**: Log shipper for collecting and forwarding log files

## ✨ Features

- **Configurable**: Use environment variables to customize for any log format
- **Multiple Log Formats**: Supports JSON, plain text, Apache logs, and custom formats
- **Hierarchical Correlation Tracking**: Advanced support for Pipeline → Process → Activity correlation IDs
- **Error Pipeline Tracing**: Easily trace errors across multi-process workflows
- **Performance Analytics**: Built-in support for performance metrics and timing data
- **Scalable**: Configurable resource allocation and performance settings
- **Docker-based**: Easy deployment with Docker Compose
- **Template Configurations**: Pre-built configs for common use cases including cpp/log-services

## 🚀 Quick Start

1. **Choose a configuration template:**
   ```bash
   # For PXPoint cpp/log-services with correlation tracking
   cp .env.pxpoint .env
   
   # For generic JSON logs
   cp .env.example .env
   
   # For simple text logs
   cp .env.simple .env
   
   # For Apache access logs
   cp .env.apache .env
   ```

2. **Customize your configuration:**
   ```bash
   # Edit the .env file with your specific settings
   nano .env
   ```

3. **Deploy the stack:**
   ```bash
   docker-compose -f docker-compose.yml up -d
   ```

4. **Access Kibana:** Open http://localhost:5601 (or your configured port)

## 📁 Directory Structure

```
elk-services/
├── config/
│   ├── elasticsearch/     # Elasticsearch configuration
│   ├── logstash/         # Logstash configuration and pipelines
│   ├── kibana/           # Kibana configuration
│   └── filebeat/         # Filebeat configuration
├── scripts/              # Management scripts
├── dashboards/           # Kibana dashboard configurations
├── logs/                 # Application log files (mounted volume)
└── docker-compose.yml    # Main orchestration file
```

## ⚙️ Configuration Options

### Core Settings
- `APPLICATION_NAME`: Name for your application (affects container and index names)
- `LOG_SOURCE_PATH`: Path to your log files (relative to elk-services directory)
- `LOG_TYPE`: Type classification for your logs
- `INDEX_PREFIX`: Elasticsearch index prefix

### Log Format Settings
- `LOG_FORMAT`: `json` or `text`
- `JSON_KEYS_UNDER_ROOT`: Extract JSON keys to root level
- `CUSTOM_GROK_PATTERN`: Custom pattern for parsing text logs

### Performance Settings
- `ELASTICSEARCH_HEAP_SIZE`: Memory allocation for Elasticsearch
- `LOGSTASH_HEAP_SIZE`: Memory allocation for Logstash
- `FILEBEAT_BUFFER_SIZE`: Buffer size for log reading

### Network Settings
- `ELASTICSEARCH_PORT`: Elasticsearch port (default: 9200)
- `KIBANA_PORT`: Kibana port (default: 5601)
- `LOGSTASH_BEATS_PORT`: Logstash beats input port (default: 5044)

## 🌐 Service URLs

- **Elasticsearch**: http://localhost:9200
- **Kibana**: http://localhost:5601  
- **Logstash**: http://localhost:9600

## 📋 Usage Examples

### Example 1: JSON Application Logs
```bash
# .env configuration
APPLICATION_NAME=myapp
LOG_SOURCE_PATH=../app/logs
LOG_FORMAT=json
INDEX_PREFIX=myapp-logs
```

### Example 2: Plain Text Service Logs
```bash
# .env configuration  
APPLICATION_NAME=service
LOG_SOURCE_PATH=./service-logs
LOG_FORMAT=text
CUSTOM_GROK_PATTERN="%{TIMESTAMP_ISO8601:timestamp} %{LOGLEVEL:level} %{GREEDYDATA:message}"
INDEX_PREFIX=service-logs
```

### Example 3: Apache Access Logs
```bash
# .env configuration
APPLICATION_NAME=webserver
LOG_SOURCE_PATH=./apache-logs  
LOG_FORMAT=text
CUSTOM_GROK_PATTERN="%{COMMONAPACHELOG}"
INDEX_PREFIX=apache-logs
```

## 🔍 Custom Log Patterns

The stack supports custom Grok patterns for parsing different log formats:

### Common Patterns
- `%{COMMONAPACHELOG}`: Apache/Nginx access logs
- `%{SYSLOGTIMESTAMP:timestamp} %{IPORHOST:host} %{PROG:program}: %{GREEDYDATA:message}`: Syslog format
- `%{TIMESTAMP_ISO8601:timestamp} %{LOGLEVEL:level} %{GREEDYDATA:message}`: Standard application logs

### Field Extraction
- `CORRELATION_ID_PATTERN`: Extract correlation IDs from log messages
- `THREAD_PATTERN`: Extract thread information

## 🔗 Hierarchical Correlation Tracking

The ELK stack includes advanced support for hierarchical correlation tracking, specifically designed for multi-process applications like PXPoint that use the cpp/log-services library.

### Correlation Hierarchy

The system supports three levels of correlation:

1. **Pipeline ID** (`pipeline-1234567890-abc123`): End-to-end business transaction
2. **Process ID** (`pipeline-1234567890-abc123-proc-orchestrator-def456`): Individual processes within a pipeline  
3. **Activity ID** (`...proc-orchestrator-def456-act-data-processing-ghi789`): Fine-grained activities within processes

### Correlation Fields in Elasticsearch

| Field | Description | Example |
|-------|-------------|---------|
| `correlation_pipeline_id` | Pipeline-level correlation ID | `pipeline-1725804615-123-abc123` |
| `correlation_process_id` | Process-level correlation ID | `pipeline-1725804615-123-abc123-proc-geo-processor-def456` |
| `correlation_activity_id` | Activity-level correlation ID | `...proc-geo-processor-def456-act-coordinate-transformation-ghi789` |
| `correlation_full_id` | Most specific correlation ID available | Automatically set to the most granular ID |
| `pipeline_timestamp` | Pipeline start timestamp (extracted) | `1725804615` |
| `process_type` | Type of process | `orchestrator`, `geo-processor`, `data-validator` |
| `activity_name` | Name of activity | `coordinate_transformation`, `data_validation` |

### Error Pipeline Tracing

Track errors across your entire multi-process pipeline:

```bash
# Find all errors in a specific pipeline
level:error AND correlation_pipeline_id:"pipeline-1725804615-123-abc123"

# Trace activity-level errors
correlation_activity_id:"*-act-data_processing-*" AND level:error

# Find failed processes
event_type:process_end AND success:false
```

### Performance Analysis

Analyze performance across correlation boundaries:

```bash
# Find slow operations across all processes
event_type:performance AND duration_ms:>1000

# Performance by process type
process_type:geo-processor AND tags:performance_metrics

# Algorithm performance comparison
algorithm:rtree AND event_type:performance
```

### Testing Correlation Tracking

Test the correlation tracking functionality:

```bash
# Run the correlation tracking test
./scripts/tools/test-correlation-tracking.sh

# Configure Kibana with sample correlation data
./scripts/tools/configure-kibana-analytics.sh
```

## 🧪 Log Generator for Testing

Include the testing profile to run a log generator:

```bash
docker-compose -f docker-compose.yml --profile testing up -d
```

## 🛠️ Management Commands

### Start the stack:
```bash
docker-compose -f docker-compose.yml up -d
```

### Stop the stack:
```bash
docker-compose -f docker-compose.yml down
```

### View logs:
```bash
docker-compose -f docker-compose.yml logs -f [service-name]
```

### Scale components:
```bash
docker-compose -f docker-compose.yml up -d --scale filebeat=2
```

## 📈 Kibana Setup

1. Access Kibana at http://localhost:5601
2. Go to Stack Management > Index Patterns
3. Create index pattern: `{INDEX_PREFIX}-*` (e.g., `myapp-logs-*`)
4. Select `@timestamp` as time field
5. Use Discover to explore logs or create visualizations

## 📋 Prerequisites

- Docker and Docker Compose installed
- At least 4GB of available RAM
- Ports 9200, 5601, 5044, and 9600 available

## 🔍 Troubleshooting

### Check service health:
```bash
docker-compose -f docker-compose.yml ps
```

### View service logs:
```bash
docker-compose -f docker-compose.yml logs elasticsearch
docker-compose -f docker-compose.yml logs logstash  
docker-compose -f docker-compose.yml logs kibana
docker-compose -f docker-compose.yml logs filebeat
```

### Common Issues

1. **Permission errors**: Ensure log directories have proper permissions
2. **Memory errors**: Adjust heap sizes in .env file
3. **Index pattern not found**: Check that logs are being processed and indexed
4. **Filebeat not reading logs**: Verify LOG_SOURCE_PATH and file permissions

## 🏗️ Architecture

- **Filebeat**: Collects and ships logs to Logstash
- **Logstash**: Processes, parses, and enriches log data
- **Elasticsearch**: Stores and indexes processed logs  
- **Kibana**: Provides web interface for log analysis and visualization

## 💾 Resource Requirements

### Minimum (Development)
- RAM: 4GB
- CPU: 2 cores
- Disk: 10GB

### Recommended (Production)  
- RAM: 8GB+
- CPU: 4+ cores
- Disk: 50GB+

Adjust heap sizes in .env file based on available resources.
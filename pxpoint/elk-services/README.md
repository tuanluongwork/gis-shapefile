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
- **Scalable**: Configurable resource allocation and performance settings
- **Docker-based**: Easy deployment with Docker Compose
- **Template Configurations**: Pre-built configs for common use cases

## 🚀 Quick Start

1. **Choose a configuration template:**
   ```bash
   # For JSON logs (like log-services)
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
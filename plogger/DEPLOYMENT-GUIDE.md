# ELK Stack Deployment Guide

## Prerequisites

Before deploying the ELK stack, ensure you have:

1. **Docker and Docker Compose installed**
2. **Docker daemon running**
3. **User added to docker group** (to avoid permission issues)
4. **At least 4GB RAM available** (recommended 8GB)
5. **Ports available**: 9200, 9300, 5601, 5044, 5000, 9600

## Quick Start

### 1. Add User to Docker Group (if needed)
```bash
sudo usermod -aG docker $USER
newgrp docker  # or logout and login again
```

### 2. Deploy ELK Stack
```bash
cd /home/tuanla/data/gis-shapefile-main/logging-solution
docker compose up -d
```

### 3. Verify Services
```bash
# Check all services are running
docker compose ps

# Check Elasticsearch health
curl http://localhost:9200/_cluster/health

# Check Kibana is accessible
curl http://localhost:5601/api/status
```

## Service Access Points

Once deployed, access the services at:

- **Elasticsearch**: http://localhost:9200
- **Kibana**: http://localhost:5601
- **Logstash API**: http://localhost:9600

## Configuration Overview

### Elasticsearch Configuration
- **Version**: 8.11.0
- **Memory**: 512MB heap size
- **Security**: Disabled (for development)
- **Storage**: Docker volume with persistence

### Logstash Configuration  
- **Input**: File monitoring, Beats, TCP (port 5000)
- **Processing**: JSON parsing, field enrichment, correlation ID extraction
- **Output**: Elasticsearch with structured indexing

### Kibana Configuration
- **Version**: 8.11.0
- **Security**: Disabled
- **Connection**: Auto-connects to Elasticsearch

### Filebeat Configuration
- **Input**: GIS server log files
- **Output**: Ships to Logstash for processing
- **Monitoring**: Health checks enabled

## Log Processing Pipeline

```
GIS Server Logs → File → Filebeat → Logstash → Elasticsearch → Kibana
```

## Integration with GIS Server

### Current Log Output
The GIS server already outputs structured JSON logs:
```json
{"timestamp":"2025-08-28T15:07:35.453152Z","level":"info","logger":"GeocodingAPI","message":"Geocoding successful | correlation_id:861383d7-5139-46f3-fda3-8ac4ff335ba0 match_type:exact confidence:1.000000 matched_address:Texas input_address:TEXAS geocode_time_ms:62.00"}
```

### Log File Location
- **File**: `/home/tuanla/data/gis-shapefile-main/logs/gis-server.log`
- **Format**: JSON with structured fields
- **Rotation**: 5MB files, 10 file retention

### Logstash Processing
The Logstash configuration automatically:
1. **Parses JSON** log entries
2. **Extracts correlation IDs** and context fields  
3. **Adds metadata** (service, environment, version)
4. **Creates performance alerts** for slow operations
5. **Indexes** to Elasticsearch with proper field mappings

## Troubleshooting

### Common Issues

1. **Docker permission denied**
   ```bash
   sudo usermod -aG docker $USER
   newgrp docker
   ```

2. **Elasticsearch fails to start** (memory)
   ```bash
   # Increase virtual memory
   sudo sysctl -w vm.max_map_count=262144
   ```

3. **Services not starting**
   ```bash
   # Check logs
   docker compose logs elasticsearch
   docker compose logs logstash
   docker compose logs kibana
   ```

4. **Port conflicts**
   ```bash
   # Check what's using ports
   sudo netstat -tulpn | grep :9200
   sudo netstat -tulpn | grep :5601
   ```

### Verification Commands

```bash
# Check all services are healthy
docker compose ps

# Test Elasticsearch
curl -X GET "http://localhost:9200/_cluster/health?pretty"

# Test Kibana
curl -X GET "http://localhost:5601/api/status"

# Check Logstash pipeline
curl -X GET "http://localhost:9600/_node/stats/pipelines?pretty"

# View logs
docker compose logs -f elasticsearch
docker compose logs -f logstash
docker compose logs -f kibana
```

## Manual Deployment Steps

If the automated script fails, deploy manually:

```bash
# 1. Pull images
docker pull elasticsearch:8.11.0
docker pull logstash:8.11.0  
docker pull kibana:8.11.0
docker pull elastic/filebeat:8.11.0

# 2. Create network
docker network create elk-network

# 3. Start services in order
docker compose up -d elasticsearch
# Wait for Elasticsearch health check to pass
docker compose up -d logstash
docker compose up -d kibana
docker compose up -d filebeat

# 4. Verify deployment
docker compose ps
```

## Configuration Files

The ELK configuration includes:

### Elasticsearch (`elk-config/elasticsearch/elasticsearch.yml`)
- Single-node setup for development
- Security disabled
- Optimized for log ingestion

### Logstash (`elk-config/logstash/logstash.conf`)
- File input monitoring
- JSON parsing and enrichment
- GIS-specific field mappings
- Performance alerting logic

### Kibana (`elk-config/kibana/kibana.yml`)
- Auto-discovery of Elasticsearch
- Pre-configured dashboards
- Security disabled for easy access

### Filebeat (`elk-config/filebeat/filebeat.yml`)
- Monitors GIS server log files
- JSON log parsing
- Retry logic for reliability

## Next Steps

1. **Deploy the stack** using the commands above
2. **Verify services** are running and healthy
3. **Access Kibana** at http://localhost:5601
4. **Create index patterns** for `gis-logs-*`
5. **Import dashboards** from `kibana-dashboards/`
6. **Start GIS server** to generate logs
7. **View logs** in Kibana Discover

The ELK stack will automatically process and index all GIS server logs, providing powerful search, visualization, and alerting capabilities for the enterprise logging solution.
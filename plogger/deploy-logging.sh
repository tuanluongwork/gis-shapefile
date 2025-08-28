#!/bin/bash

set -e

echo "========================================="
echo "GIS Logging Infrastructure Deployment"
echo "Phase 1: Foundation Setup"
echo "========================================="

# Create necessary directories
echo "Creating directory structure..."
mkdir -p logs
mkdir -p elk-config/{elasticsearch,logstash,kibana,filebeat}
mkdir -p kibana-dashboards

# Set proper permissions
echo "Setting permissions..."
chmod +x scripts/deploy-logging.sh
chmod 664 elk-config/filebeat/filebeat.yml

# Create logs directory with proper permissions
sudo mkdir -p /var/log/gis-server
sudo chown -R $USER:$USER /var/log/gis-server

# Pull required Docker images
echo "Pulling Docker images..."
docker pull elasticsearch:8.11.0
docker pull logstash:8.11.0
docker pull kibana:8.11.0
docker pull elastic/filebeat:8.11.0

# Start the ELK stack
echo "Starting ELK stack..."
docker-compose up -d

# Wait for Elasticsearch to be ready
echo "Waiting for Elasticsearch to be ready..."
until curl -f http://localhost:9200/_cluster/health > /dev/null 2>&1; do
    echo "Waiting for Elasticsearch..."
    sleep 10
done

# Wait for Kibana to be ready
echo "Waiting for Kibana to be ready..."
until curl -f http://localhost:5601/api/status > /dev/null 2>&1; do
    echo "Waiting for Kibana..."
    sleep 10
done

# Import Kibana dashboards
echo "Importing Kibana dashboards..."
if [ -f kibana-dashboards/gis-monitoring-dashboard.json ]; then
    curl -X POST "http://localhost:5601/api/saved_objects/_import" \
         -H "kbn-xsrf: true" \
         -H "Content-Type: application/json" \
         --form file=@kibana-dashboards/gis-monitoring-dashboard.json
fi

# Create index template in Elasticsearch
echo "Creating Elasticsearch index template..."
curl -X PUT "http://localhost:9200/_index_template/gis-logs-template" \
     -H "Content-Type: application/json" \
     -d '{
       "index_patterns": ["gis-logs-*"],
       "priority": 100,
       "template": {
         "settings": {
           "number_of_shards": 1,
           "number_of_replicas": 0,
           "index.refresh_interval": "5s"
         },
         "mappings": {
           "properties": {
             "@timestamp": { "type": "date" },
             "level": { "type": "keyword" },
             "logger": { "type": "keyword" },
             "message": { "type": "text" },
             "service": { "type": "keyword" },
             "environment": { "type": "keyword" },
             "correlation_id": { "type": "keyword" },
             "context": {
               "type": "object",
               "properties": {
                 "fips": { "type": "keyword" },
                 "job_id": { "type": "keyword" },
                 "process_step": { "type": "keyword" },
                 "execution_time_ms": { "type": "float" },
                 "memory_usage_mb": { "type": "float" },
                 "response_time_ms": { "type": "float" },
                 "input_address": { "type": "text" },
                 "matched_address": { "type": "text" },
                 "confidence": { "type": "float" }
               }
             }
           }
         }
       }
     }'

echo ""
echo "========================================="
echo "Deployment Complete!"
echo "========================================="
echo "Services available at:"
echo "  - Elasticsearch: http://localhost:9200"
echo "  - Kibana: http://localhost:5601"
echo "  - Logstash: http://localhost:9600"
echo ""
echo "Next steps:"
echo "1. Build and run the GIS server with: make && ./gis-server -d data -p 8080"
echo "2. View logs in Kibana at: http://localhost:5601"
echo "3. Check service status with: docker-compose ps"
echo ""
echo "To stop services: docker-compose down"
echo "To view logs: docker-compose logs -f [service-name]"
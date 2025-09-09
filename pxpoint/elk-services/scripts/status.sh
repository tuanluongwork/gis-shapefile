#!/bin/bash

# ELK Stack Status Script
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "📊 ELK Stack Status"
echo "==================="

cd "$PROJECT_DIR"

# Check if services are running
echo "🔍 Container Status:"
docker-compose ps

echo ""
echo "🌐 Service Health Checks:"

# Check Elasticsearch
if curl -s http://localhost:9200/_cluster/health > /dev/null 2>&1; then
    echo "  ✅ Elasticsearch: Running (http://localhost:9200)"
    CLUSTER_HEALTH=$(curl -s http://localhost:9200/_cluster/health | jq -r '.status' 2>/dev/null || echo "unknown")
    echo "     Cluster Health: $CLUSTER_HEALTH"
else
    echo "  ❌ Elasticsearch: Not responding"
fi

# Check Logstash
if curl -s http://localhost:9600/_node/stats > /dev/null 2>&1; then
    echo "  ✅ Logstash: Running (http://localhost:9600)"
else
    echo "  ❌ Logstash: Not responding"
fi

# Check Kibana
if curl -s http://localhost:5601/api/status > /dev/null 2>&1; then
    echo "  ✅ Kibana: Running (http://localhost:5601)"
else
    echo "  ❌ Kibana: Not responding"
fi

# Check Filebeat (indirectly by checking if container is running)
if docker-compose ps filebeat | grep -q "Up"; then
    echo "  ✅ Filebeat: Running"
else
    echo "  ❌ Filebeat: Not running"
fi

echo ""
echo "📁 Disk Usage:"
docker system df

if command -v docker-compose &> /dev/null; then
    echo ""
    echo "💾 Volume Information:"
    docker volume ls | grep elk || echo "  No ELK volumes found"
fi
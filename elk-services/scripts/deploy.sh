#!/bin/bash

# ELK Stack Deployment Script
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "🚀 Starting ELK Stack deployment..."

# Check if Docker is running
if ! docker info > /dev/null 2>&1; then
    echo "❌ Docker is not running. Please start Docker and try again."
    exit 1
fi

# Check if Docker Compose is available
if ! command -v docker-compose &> /dev/null; then
    echo "❌ Docker Compose is not installed. Please install Docker Compose and try again."
    exit 1
fi

# Load environment variables
if [ -f "$PROJECT_DIR/.env" ]; then
    source "$PROJECT_DIR/.env"
fi

# Create log source directory if it doesn't exist and it's a local path
if [[ "${LOG_SOURCE_PATH:-/tmp/pxpoint-logs}" =~ ^\.\/ ]] || [[ "${LOG_SOURCE_PATH:-/tmp/pxpoint-logs}" =~ ^[^/] ]]; then
    # It's a relative path, create it relative to PROJECT_DIR
    mkdir -p "$PROJECT_DIR/${LOG_SOURCE_PATH:-logs}"
elif [[ "${LOG_SOURCE_PATH:-/tmp/pxpoint-logs}" =~ ^/ ]]; then
    # It's an absolute path, create it directly
    mkdir -p "${LOG_SOURCE_PATH:-/tmp/pxpoint-logs}"
fi

# Set proper permissions for Elasticsearch data directory
echo "📁 Setting up directories and permissions..."
docker run --rm -v elk_elasticsearch_data:/data alpine:latest chown -R 1000:1000 /data || true

# Pull latest images
echo "📥 Pulling latest ELK Stack images..."
cd "$PROJECT_DIR"
docker-compose pull

# Start the ELK stack
echo "🔄 Starting ELK Stack services..."
docker-compose up -d

# Wait for Elasticsearch to be ready
echo "⏳ Waiting for Elasticsearch to be ready..."
timeout=300
counter=0
while ! curl -s http://localhost:9200/_cluster/health > /dev/null; do
    if [ $counter -ge $timeout ]; then
        echo "❌ Elasticsearch failed to start within ${timeout} seconds"
        docker-compose logs elasticsearch
        exit 1
    fi
    echo "  Waiting for Elasticsearch... ($counter/$timeout)"
    sleep 5
    ((counter+=5))
done

echo "✅ Elasticsearch is ready!"

# Wait for Kibana to be ready
echo "⏳ Waiting for Kibana to be ready..."
counter=0
while ! curl -s http://localhost:5601/api/status > /dev/null; do
    if [ $counter -ge $timeout ]; then
        echo "❌ Kibana failed to start within ${timeout} seconds"
        docker-compose logs kibana
        exit 1
    fi
    echo "  Waiting for Kibana... ($counter/$timeout)"
    sleep 5
    ((counter+=5))
done

echo "✅ Kibana is ready!"

# Configure Kibana Analytics
echo "🔧 Configuring Kibana Analytics..."
if [ -f "$SCRIPT_DIR/tools/configure-kibana-analytics.sh" ]; then
    "$SCRIPT_DIR/tools/configure-kibana-analytics.sh"
else
    echo "⚠️  Analytics configuration script not found, manual setup required"
fi

# Check all services status
echo "📊 Checking services status..."
docker-compose ps

echo ""
echo "🎉 ELK Stack deployment completed successfully!"
echo ""
echo "📍 Service URLs:"
echo "   • Elasticsearch: http://localhost:9200"
echo "   • Kibana:        http://localhost:5601"
echo "   • Logstash:      http://localhost:9600"
echo ""
echo "📁 Log files should be placed in: ${LOG_SOURCE_PATH:-/tmp/pxpoint-logs}"
echo ""
echo "🔍 Quick Start Guide:"
echo "   1. Open Kibana: http://localhost:5601"
echo "   2. Go to Analytics → Discover"
echo "   3. Select 'pxp-logs-*' data view"
echo "   4. Set time range to 'Last 24 hours'"
echo "   5. Explore your logs!"
echo ""
echo "🔧 To stop the services: ./scripts/stop.sh"
echo "🔄 To restart the services: ./scripts/restart.sh"
echo "📜 To view logs: ./scripts/logs.sh [service_name]"
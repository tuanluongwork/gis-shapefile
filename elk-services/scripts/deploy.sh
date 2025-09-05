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

# Create logs directory if it doesn't exist
mkdir -p "$PROJECT_DIR/logs"

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
echo "📁 Log files should be placed in: $PROJECT_DIR/logs"
echo ""
echo "🔧 To stop the services: ./scripts/stop.sh"
echo "🔄 To restart the services: ./scripts/restart.sh"
echo "📜 To view logs: ./scripts/logs.sh [service_name]"
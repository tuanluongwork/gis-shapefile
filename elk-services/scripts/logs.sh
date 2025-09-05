#!/bin/bash

# ELK Stack Logs Script
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

SERVICE_NAME=${1:-""}

if [ -z "$SERVICE_NAME" ]; then
    echo "📜 Showing logs for all ELK Stack services..."
    echo "💡 Use './scripts/logs.sh <service_name>' to view specific service logs"
    echo "   Available services: elasticsearch, logstash, kibana, filebeat"
    echo ""
    cd "$PROJECT_DIR"
    docker-compose logs --tail=100 -f
else
    case $SERVICE_NAME in
        elasticsearch|logstash|kibana|filebeat)
            echo "📜 Showing logs for $SERVICE_NAME..."
            cd "$PROJECT_DIR"
            docker-compose logs --tail=100 -f "$SERVICE_NAME"
            ;;
        *)
            echo "❌ Invalid service name: $SERVICE_NAME"
            echo "Available services: elasticsearch, logstash, kibana, filebeat"
            exit 1
            ;;
    esac
fi
#!/bin/bash

# ELK Stack Cleanup Script
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "🗑️  ELK Stack Cleanup Script"
echo "This will remove all containers, volumes, and data!"
echo ""
read -p "Are you sure you want to continue? (y/N): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Operation cancelled."
    exit 0
fi

echo "🛑 Stopping and removing ELK Stack..."

cd "$PROJECT_DIR"

# Stop and remove containers
docker-compose down -v --remove-orphans

# Remove volumes
docker volume rm elk_elasticsearch_data 2>/dev/null || true

# Remove any dangling containers
docker container prune -f

echo ""
echo "✅ ELK Stack cleanup completed!"
echo "💡 To deploy again: ./scripts/deploy.sh"
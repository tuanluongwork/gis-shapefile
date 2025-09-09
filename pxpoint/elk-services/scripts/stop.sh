#!/bin/bash

# ELK Stack Stop Script
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "🛑 Stopping ELK Stack services..."

cd "$PROJECT_DIR"
docker-compose down

echo "✅ ELK Stack services stopped successfully!"
echo ""
echo "💡 To start the services again: ./scripts/deploy.sh"
echo "🗑️  To remove all data: ./scripts/cleanup.sh"
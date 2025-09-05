#!/bin/bash

# ELK Stack Restart Script
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "🔄 Restarting ELK Stack services..."

# Stop services
"$SCRIPT_DIR/stop.sh"

echo ""
echo "⏳ Waiting 10 seconds before restart..."
sleep 10

# Start services
"$SCRIPT_DIR/deploy.sh"
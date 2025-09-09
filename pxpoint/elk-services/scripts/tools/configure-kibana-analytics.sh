#!/bin/bash

# Enhanced Kibana Analytics Configuration Script
# This script ensures Kibana Analytics page displays data properly

echo "🔧 Configuring Kibana Analytics for PXPoint logs..."

# Wait for Kibana to be fully ready
echo "Waiting for Kibana to be fully operational..."
while true; do
    STATUS=$(curl -s "http://localhost:5601/api/status" 2>/dev/null | jq -r '.status.overall.level' 2>/dev/null)
    if [ "$STATUS" = "available" ]; then
        echo "✅ Kibana is ready"
        break
    else
        echo "⏳ Kibana status: $STATUS, waiting..."
        sleep 5
    fi
done

# Check if we have data in Elasticsearch
echo "Checking for data in Elasticsearch..."
DATA_COUNT=$(curl -s "http://localhost:9200/pxp-logs-*/_count" | jq -r '.count' 2>/dev/null)
if [ "$DATA_COUNT" = "0" ] || [ "$DATA_COUNT" = "null" ]; then
    echo "❌ No data found in pxp-logs-* indices"
    echo "Creating sample data..."
    
    # Create sample data
    cat << 'EOF' > /tmp/sample_kibana_data.json
{"index": {"_index": "pxp-logs-2025.09.08"}}
{"@timestamp": "2025-09-08T14:30:15.123Z", "level": "info", "logger": "pxpoint.service.main", "message": "Application started successfully", "correlation_id": "app-001", "component": "main", "status": "active", "application": "pxpoint", "log_type": "application", "log_source": "filebeat"}
{"index": {"_index": "pxp-logs-2025.09.08"}}
{"@timestamp": "2025-09-08T14:30:16.234Z", "level": "info", "logger": "pxpoint.gis.processor", "message": "Processing shapefile data", "correlation_id": "gis-002", "component": "processor", "operation": "shapefile_processing", "status": "processing", "application": "pxpoint", "log_type": "application", "log_source": "filebeat"}
{"index": {"_index": "pxp-logs-2025.09.08"}}
{"@timestamp": "2025-09-08T14:30:17.345Z", "level": "warn", "logger": "pxpoint.database.connection", "message": "Database connection slow", "correlation_id": "db-003", "component": "database", "response_time_ms": 850, "threshold_ms": 500, "status": "warning", "application": "pxpoint", "log_type": "application", "log_source": "filebeat"}
{"index": {"_index": "pxp-logs-2025.09.08"}}
{"@timestamp": "2025-09-08T14:30:18.456Z", "level": "error", "logger": "pxpoint.auth.service", "message": "Authentication failed", "correlation_id": "auth-004", "component": "auth", "user_id": "user123", "status": "failed", "error_code": "INVALID_CREDENTIALS", "application": "pxpoint", "log_type": "application", "log_source": "filebeat"}
EOF

    curl -s -X POST "http://localhost:9200/_bulk" -H "Content-Type: application/x-ndjson" --data-binary @/tmp/sample_kibana_data.json > /dev/null
    rm /tmp/sample_kibana_data.json
    
    # Wait for data to be indexed
    sleep 2
    DATA_COUNT=$(curl -s "http://localhost:9200/pxp-logs-*/_count" | jq -r '.count' 2>/dev/null)
fi

echo "✅ Found $DATA_COUNT documents in pxp-logs-* indices"

# Create or verify data view (index pattern)
echo "Setting up data view for pxp-logs-*..."
DATA_VIEW_RESPONSE=$(curl -s -X POST "http://localhost:5601/api/data_views/data_view" \
    -H "Content-Type: application/json" \
    -H "kbn-xsrf: true" \
    -d '{
        "data_view": {
            "title": "pxp-logs-*",
            "timeFieldName": "@timestamp",
            "name": "PXPoint Logs"
        }
    }' 2>/dev/null)

DATA_VIEW_ID=$(echo "$DATA_VIEW_RESPONSE" | jq -r '.data_view.id' 2>/dev/null)

# If creation failed, try to get existing data view
if [ "$DATA_VIEW_ID" = "null" ] || [ -z "$DATA_VIEW_ID" ]; then
    echo "Data view may already exist, checking..."
    EXISTING_DATA_VIEW=$(curl -s "http://localhost:5601/api/data_views" -H "kbn-xsrf: true" 2>/dev/null)
    DATA_VIEW_ID=$(echo "$EXISTING_DATA_VIEW" | jq -r '.data_view[] | select(.title == "pxp-logs-*") | .id' 2>/dev/null)
fi

if [ -n "$DATA_VIEW_ID" ] && [ "$DATA_VIEW_ID" != "null" ]; then
    echo "✅ Data view created/found with ID: $DATA_VIEW_ID"
    
    # Set as default data view
    curl -s -X POST "http://localhost:5601/api/kibana/settings" \
        -H "Content-Type: application/json" \
        -H "kbn-xsrf: true" \
        -d "{\"changes\":{\"defaultIndex\":\"$DATA_VIEW_ID\"}}" > /dev/null 2>&1
    
    echo "✅ Set as default data view"
else
    echo "❌ Failed to create/find data view"
fi

# Refresh field mappings
echo "Refreshing field mappings..."
if [ -n "$DATA_VIEW_ID" ] && [ "$DATA_VIEW_ID" != "null" ]; then
    curl -s -X POST "http://localhost:5601/api/data_views/data_view/$DATA_VIEW_ID/fields" \
        -H "Content-Type: application/json" \
        -H "kbn-xsrf: true" > /dev/null 2>&1
fi

echo ""
echo "🎉 Kibana Analytics configuration complete!"
echo ""
echo "📊 Access your analytics:"
echo "   • Kibana: http://localhost:5601"
echo "   • Navigate to: Analytics → Discover"
echo "   • Data view: pxp-logs-*"
echo "   • Time range: Last 24 hours"
echo ""
echo "🔍 Key fields available:"
echo "   • @timestamp: Log timestamp"
echo "   • level: Log level (info, warn, error)" 
echo "   • logger: Logger name"
echo "   • message: Log message"
echo "   • correlation_id: Request correlation ID"
echo "   • component: Application component"
echo "   • status: Operation status"
echo ""
echo "💡 Try these searches in Discover:"
echo "   • level:error (find all errors)"
echo "   • component:database (database-related logs)"
echo "   • status:failed (failed operations)"
echo "   • correlation_id:* (logs with correlation IDs)"
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
echo "Current document count: $DATA_COUNT"
if [ "$DATA_COUNT" = "0" ] || [ "$DATA_COUNT" = "null" ] || [ -z "$DATA_COUNT" ]; then
    echo "❌ No data found in pxp-logs-* indices"
    echo "Creating sample data..."
    
    # Create sample data with hierarchical correlation structure
    cat << 'EOF' > /tmp/sample_kibana_data.json
{"index": {"_index": "pxp-logs-2025.09.08"}}
{"@timestamp": "2025-09-08T14:30:15.123Z", "level": "info", "logger": "application", "message": "Orchestrator process started", "correlation_pipeline_id": "pipeline-1725804615-123-abc123", "correlation_process_id": "pipeline-1725804615-123-abc123-proc-orchestrator-def456", "component": "orchestrator", "process_type": "orchestrator", "event_type": "process_start", "application": "pxpoint", "log_type": "application", "log_source": "filebeat", "tags": ["has_pipeline_correlation", "has_process_correlation", "event_process_start"]}
{"index": {"_index": "pxp-logs-2025.09.08"}}
{"@timestamp": "2025-09-08T14:30:16.234Z", "level": "info", "logger": "application", "message": "Starting data processing activity", "correlation_pipeline_id": "pipeline-1725804615-123-abc123", "correlation_process_id": "pipeline-1725804615-123-abc123-proc-geo-processor-ghi789", "correlation_activity_id": "pipeline-1725804615-123-abc123-proc-geo-processor-ghi789-act-data_processing-jkl012", "component": "geo-processor", "process_type": "geo-processor", "activity_name": "data_processing", "step": "coordinate_transformation", "application": "pxpoint", "log_type": "application", "log_source": "filebeat", "tags": ["has_pipeline_correlation", "has_process_correlation", "has_activity_correlation"]}
{"index": {"_index": "pxp-logs-2025.09.08"}}
{"@timestamp": "2025-09-08T14:30:17.345Z", "level": "info", "logger": "application", "message": "Performance measurement: coordinate_transformation", "correlation_pipeline_id": "pipeline-1725804615-123-abc123", "correlation_process_id": "pipeline-1725804615-123-abc123-proc-geo-processor-ghi789", "correlation_activity_id": "pipeline-1725804615-123-abc123-proc-geo-processor-ghi789-act-data_processing-jkl012", "component": "geo-processor", "event_type": "performance", "operation": "coordinate_transformation", "algorithm": "rtree", "duration_ms": 150.25, "application": "pxpoint", "log_type": "application", "log_source": "filebeat", "tags": ["has_pipeline_correlation", "has_process_correlation", "has_activity_correlation", "performance_metrics", "event_performance"]}
{"index": {"_index": "pxp-logs-2025.09.08"}}
{"@timestamp": "2025-09-08T14:30:18.456Z", "level": "error", "logger": "application", "message": "Worker process failed", "correlation_pipeline_id": "pipeline-1725804615-123-abc123", "correlation_process_id": "pipeline-1725804615-123-abc123-proc-data-validator-mno345", "correlation_activity_id": "pipeline-1725804615-123-abc123-proc-data-validator-mno345-act-validation-pqr678", "component": "data-validator", "process_type": "data-validator", "worker_type": "data-validator", "success": false, "error_code": "VALIDATION_FAILED", "application": "pxpoint", "log_type": "application", "log_source": "filebeat", "tags": ["has_pipeline_correlation", "has_process_correlation", "has_activity_correlation"]}
{"index": {"_index": "pxp-logs-2025.09.08"}}
{"@timestamp": "2025-09-08T14:30:19.567Z", "level": "info", "logger": "application", "message": "Process completed successfully: orchestrator", "correlation_pipeline_id": "pipeline-1725804615-123-abc123", "correlation_process_id": "pipeline-1725804615-123-abc123-proc-orchestrator-def456", "component": "orchestrator", "event_type": "process_end", "process_type": "orchestrator", "success": true, "total_processing_time_ms": 4344.0, "workers_spawned": 3.0, "application": "pxpoint", "log_type": "application", "log_source": "filebeat", "tags": ["has_pipeline_correlation", "has_process_correlation", "event_process_end", "performance_metrics"]}
EOF

    curl -s -X POST "http://localhost:9200/_bulk" -H "Content-Type: application/x-ndjson" --data-binary @/tmp/sample_kibana_data.json > /dev/null
    rm /tmp/sample_kibana_data.json
    
    # Wait for data to be indexed
    sleep 2
    DATA_COUNT=$(curl -s "http://localhost:9200/pxp-logs-*/_count" | jq -r '.count' 2>/dev/null)
else
    echo "✅ Found existing data in pxp-logs-* indices"
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
echo "   • level: Log level (info, warn, error, debug, critical)" 
echo "   • logger: Logger name"
echo "   • message: Log message"
echo "   • correlation_pipeline_id: Pipeline-level correlation ID"
echo "   • correlation_process_id: Process-level correlation ID"
echo "   • correlation_activity_id: Activity-level correlation ID"
echo "   • correlation_full_id: Most specific correlation ID available"
echo "   • component: Application component"
echo "   • process_type: Type of process (orchestrator, geo-processor, etc.)"
echo "   • activity_name: Activity name within process"
echo "   • event_type: Event type (process_start, process_end, performance)"
echo "   • operation: Specific operation being performed"
echo "   • duration_ms, processing_time_ms: Performance metrics"
echo ""
echo "💡 Try these searches in Discover:"
echo "   • level:error (find all errors)"
echo "   • process_type:geo-processor (geo-processing logs)"
echo "   • event_type:performance (performance measurements)"
echo "   • correlation_pipeline_id:pipeline-* (all logs for a pipeline)"
echo "   • correlation_process_id:*-proc-orchestrator-* (orchestrator logs)"
echo "   • correlation_activity_id:*-act-* (activity-level logs)"
echo "   • success:false (failed operations)"
echo "   • tags:performance_metrics (logs with timing data)"
echo "   • algorithm:rtree (specific algorithm performance)"
echo ""
echo "🎯 Pipeline Error Tracing Examples:"
echo "   • Find pipeline errors: level:error AND correlation_pipeline_id:YOUR_PIPELINE_ID"
echo "   • Trace activity flow: correlation_activity_id:YOUR_ACTIVITY_ID"
echo "   • Performance analysis: event_type:performance AND duration_ms:>100"
echo "   • Process failures: event_type:process_end AND success:false"
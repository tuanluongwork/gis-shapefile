#!/bin/bash

# Test script for correlation tracking in ELK services
# This script validates that the updated ELK configuration properly handles
# hierarchical correlation IDs from cpp/log-services

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

echo "🧪 Testing ELK Services Correlation Tracking"
echo "============================================="

# Check if services are running
echo "🔍 Checking service status..."
if ! docker-compose ps | grep -q "Up"; then
    echo "❌ ELK services not running. Starting services..."
    ./scripts/deploy.sh
fi

# Wait for services to be ready
echo "⏳ Waiting for services to be ready..."
sleep 10

# Create test log data via Filebeat/Logstash pipeline
echo "📝 Creating test log data with correlation IDs..."
TEST_PIPELINE_ID="pipeline-$(date +%s)-test-$(openssl rand -hex 3)"
TEST_PROCESS_ID="${TEST_PIPELINE_ID}-proc-test-processor-$(openssl rand -hex 4)"
TEST_ACTIVITY_ID="${TEST_PROCESS_ID}-act-test-activity-$(openssl rand -hex 4)"

# Create test log files in the monitored directory
mkdir -p /tmp/pxpoint-logs
LOG_FILE="/tmp/pxpoint-logs/correlation-test-$(date +%s).log"

echo "📤 Writing test logs to $LOG_FILE for Filebeat to process..."
cat << EOF > "$LOG_FILE"
{"timestamp":"$(date -u +%Y-%m-%dT%H:%M:%S.%3NZ)","level":"info","logger":"application","thread":"12345","process_id":"12345","message":"Test orchestrator process started | process_id:${TEST_PROCESS_ID} pipeline_id:${TEST_PIPELINE_ID}  | process_type:test-orchestrator event_type:process_start version:1.0.0 ","correlation_pipeline_id":"${TEST_PIPELINE_ID}","correlation_process_id":"${TEST_PROCESS_ID}","component":"test-orchestrator","process_type":"test-orchestrator","event_type":"process_start"}

{"timestamp":"$(date -u +%Y-%m-%dT%H:%M:%S.%3NZ)","level":"info","logger":"application","thread":"12345","process_id":"12345","message":"Starting test activity | activity_id:${TEST_ACTIVITY_ID} process_id:${TEST_PROCESS_ID} pipeline_id:${TEST_PIPELINE_ID} ","correlation_pipeline_id":"${TEST_PIPELINE_ID}","correlation_process_id":"${TEST_PROCESS_ID}","correlation_activity_id":"${TEST_ACTIVITY_ID}","component":"test-processor","activity_name":"test_activity"}

{"timestamp":"$(date -u +%Y-%m-%dT%H:%M:%S.%3NZ)","level":"info","logger":"application","thread":"12345","process_id":"12345","message":"Performance measurement: test_operation | activity_id:${TEST_ACTIVITY_ID} process_id:${TEST_PROCESS_ID} pipeline_id:${TEST_PIPELINE_ID}  | operation:test_operation event_type:performance algorithm:test duration_ms:125.50 ","correlation_pipeline_id":"${TEST_PIPELINE_ID}","correlation_process_id":"${TEST_PROCESS_ID}","correlation_activity_id":"${TEST_ACTIVITY_ID}","event_type":"performance","operation":"test_operation","algorithm":"test","duration_ms":125.50}

{"timestamp":"$(date -u +%Y-%m-%dT%H:%M:%S.%3NZ)","level":"error","logger":"application","thread":"12345","process_id":"12345","message":"Test error occurred | activity_id:${TEST_ACTIVITY_ID} process_id:${TEST_PROCESS_ID} pipeline_id:${TEST_PIPELINE_ID}  | error_code:TEST_ERROR component:test-component ","correlation_pipeline_id":"${TEST_PIPELINE_ID}","correlation_process_id":"${TEST_PROCESS_ID}","correlation_activity_id":"${TEST_ACTIVITY_ID}","error_code":"TEST_ERROR","component":"test-component"}

{"timestamp":"$(date -u +%Y-%m-%dT%H:%M:%S.%3NZ)","level":"info","logger":"application","thread":"12345","process_id":"12345","message":"Process completed successfully: test-orchestrator | process_id:${TEST_PROCESS_ID} pipeline_id:${TEST_PIPELINE_ID}  | success:true event_type:process_end total_processing_time_ms:500.25 ","correlation_pipeline_id":"${TEST_PIPELINE_ID}","correlation_process_id":"${TEST_PROCESS_ID}","event_type":"process_end","success":true,"total_processing_time_ms":500.25}
EOF

echo "📤 Waiting for Filebeat to process logs and Logstash to index them..."

# Wait for Filebeat to pick up the file and Logstash to process it
sleep 15

# Test correlation tracking queries
echo "🔍 Testing correlation ID extraction and indexing..."

# Test 1: Check if documents were indexed
DOC_COUNT=$(curl -s "http://localhost:9200/pxp-logs-*/_count" | jq -r '.count' 2>/dev/null || echo "0")
echo "✓ Total documents indexed: $DOC_COUNT"

# Test 2: Search by pipeline correlation ID
PIPELINE_RESULTS=$(curl -s -X GET "http://localhost:9200/pxp-logs-*/_search" \
    -H "Content-Type: application/json" \
    -d "{
        \"query\": {
            \"bool\": {
                \"should\": [
                    {\"term\": {\"correlation_pipeline_id.keyword\": \"${TEST_PIPELINE_ID}\"}},
                    {\"wildcard\": {\"message\": \"*pipeline_id:${TEST_PIPELINE_ID}*\"}}
                ]
            }
        }
    }" | jq -r '.hits.total.value' 2>/dev/null || echo "0")

echo "✓ Documents found by pipeline ID: $PIPELINE_RESULTS"

# Test 3: Search by process correlation ID  
PROCESS_RESULTS=$(curl -s -X GET "http://localhost:9200/pxp-logs-*/_search" \
    -H "Content-Type: application/json" \
    -d "{
        \"query\": {
            \"bool\": {
                \"should\": [
                    {\"term\": {\"correlation_process_id.keyword\": \"${TEST_PROCESS_ID}\"}},
                    {\"wildcard\": {\"message\": \"*process_id:${TEST_PROCESS_ID}*\"}}
                ]
            }
        }
    }" | jq -r '.hits.total.value' 2>/dev/null || echo "0")

echo "✓ Documents found by process ID: $PROCESS_RESULTS"

# Test 4: Search by activity correlation ID
ACTIVITY_RESULTS=$(curl -s -X GET "http://localhost:9200/pxp-logs-*/_search" \
    -H "Content-Type: application/json" \
    -d "{
        \"query\": {
            \"bool\": {
                \"should\": [
                    {\"term\": {\"correlation_activity_id.keyword\": \"${TEST_ACTIVITY_ID}\"}},
                    {\"wildcard\": {\"message\": \"*activity_id:${TEST_ACTIVITY_ID}*\"}}
                ]
            }
        }
    }" | jq -r '.hits.total.value' 2>/dev/null || echo "0")

echo "✓ Documents found by activity ID: $ACTIVITY_RESULTS"

# Test 5: Performance metrics extraction
PERF_RESULTS=$(curl -s -X GET "http://localhost:9200/pxp-logs-*/_search" \
    -H "Content-Type: application/json" \
    -d '{
        "query": {
            "bool": {
                "must": [
                    {"term": {"event_type": "performance"}},
                    {"exists": {"field": "duration_ms"}}
                ]
            }
        }
    }' | jq -r '.hits.total.value')

echo "✓ Documents with performance metrics: $PERF_RESULTS"

# Test 6: Error tracing by correlation
ERROR_RESULTS=$(curl -s -X GET "http://localhost:9200/pxp-logs-*/_search" \
    -H "Content-Type: application/json" \
    -d "{
        \"query\": {
            \"bool\": {
                \"must\": [
                    {\"term\": {\"level\": \"error\"}},
                    {\"term\": {\"correlation_pipeline_id.keyword\": \"${TEST_PIPELINE_ID}\"}}
                ]
            }
        }
    }" | jq -r '.hits.total.value' 2>/dev/null || echo "0")

echo "✓ Error documents traceable by correlation: $ERROR_RESULTS"

# Cleanup test files
rm -f "$LOG_FILE"

echo ""
echo "🎯 Correlation Tracking Test Summary:"
echo "======================================"
echo "Pipeline ID: $TEST_PIPELINE_ID"
echo "Process ID: $TEST_PROCESS_ID" 
echo "Activity ID: $TEST_ACTIVITY_ID"
echo ""
echo "Results:"
echo "• Pipeline-level tracking: $PIPELINE_RESULTS documents"
echo "• Process-level tracking: $PROCESS_RESULTS documents"
echo "• Activity-level tracking: $ACTIVITY_RESULTS documents"
echo "• Performance metrics: $PERF_RESULTS documents"
echo "• Error tracing: $ERROR_RESULTS documents"
echo ""

if [[ $PIPELINE_RESULTS -gt 0 && $PROCESS_RESULTS -gt 0 && $ACTIVITY_RESULTS -gt 0 ]]; then
    echo "✅ Correlation tracking test PASSED!"
    echo ""
    echo "🔗 Kibana Dashboard Access:"
    echo "   http://localhost:5601/app/discover"
    echo ""
    echo "📊 Sample Queries for Pipeline Error Tracing:"
    echo "   • Full pipeline trace: correlation_pipeline_id:\"${TEST_PIPELINE_ID}\""
    echo "   • Process-specific logs: correlation_process_id:\"${TEST_PROCESS_ID}\""
    echo "   • Activity-level trace: correlation_activity_id:\"${TEST_ACTIVITY_ID}\""
    echo "   • Pipeline errors: level:error AND message:*pipeline_id\\:${TEST_PIPELINE_ID}*"
else
    echo "❌ Correlation tracking test FAILED!"
    echo "Check Logstash configuration and field mappings."
fi
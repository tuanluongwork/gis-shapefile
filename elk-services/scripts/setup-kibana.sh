#!/bin/bash

# Wait for Kibana to be ready
echo "Waiting for Kibana to be ready..."
while ! curl -s http://localhost:5601/api/status | grep -q "available"; do
  echo "Kibana not ready yet, waiting 10 seconds..."
  sleep 10
done

echo "Kibana is ready. Setting up index patterns and dashboards..."

# Create index pattern
echo "Creating index pattern for pxp-logs-*..."
curl -X POST "localhost:5601/api/data_views/data_view" \
  -H "Content-Type: application/json" \
  -H "kbn-xsrf: true" \
  -d '{
    "data_view": {
      "title": "pxp-logs-*",
      "timeFieldName": "@timestamp"
    }
  }' 2>/dev/null

# Check if index pattern was created
sleep 2
INDEX_PATTERN_ID=$(curl -s "localhost:5601/api/data_views" -H "kbn-xsrf: true" | jq -r '.data_view[] | select(.title == "pxp-logs-*") | .id' 2>/dev/null)

if [ ! -z "$INDEX_PATTERN_ID" ]; then
  echo "Index pattern created successfully with ID: $INDEX_PATTERN_ID"
  
  # Set as default index pattern
  echo "Setting as default index pattern..."
  curl -X POST "localhost:5601/api/kibana/settings/defaultIndex" \
    -H "Content-Type: application/json" \
    -H "kbn-xsrf: true" \
    -d "{\"value\":\"$INDEX_PATTERN_ID\"}" 2>/dev/null
    
  echo "Default index pattern set successfully"
else
  echo "Failed to create index pattern"
  exit 1
fi

echo "Kibana setup complete!"
echo "You can now access Kibana at: http://localhost:5601"
echo "Go to Analytics -> Discover to explore your logs"
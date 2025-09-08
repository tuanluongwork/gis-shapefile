#!/bin/bash

# PxPoint Multi-Process Correlation Test Script
# This demonstrates how the correlation system works across multiple processes

echo "=== PxPoint Multi-Process Correlation Test ==="
echo "Simulating a PxPoint pipeline with multiple processes"
echo

# Build the C++ dummy process if needed
cd /home/tuanla/data/gis-shapefile-main/pxpoint/build

# Clear any existing pipeline environment
unset PXPOINT_PIPELINE_ID
unset PXPOINT_PROCESS_ID

echo "1. Starting first process (ParcelProcessor for FIPS 01001)..."
echo "   This will create the pipeline correlation ID"
echo
./bin/dummy_parcel_processor 01001
echo
echo "   Pipeline environment after first process:"
echo "   PXPOINT_PIPELINE_ID: $PXPOINT_PIPELINE_ID"
echo "   PXPOINT_PROCESS_ID: $PXPOINT_PROCESS_ID"
echo

echo "2. Starting second process (ParcelProcessor for FIPS 01002)..."
echo "   This should inherit the pipeline correlation ID"
echo
./bin/dummy_parcel_processor 01002
echo

echo "3. Starting third process (ParcelProcessor for FIPS 01003)..."
echo "   This should also inherit the same pipeline correlation ID"
echo
./bin/dummy_parcel_processor 01003
echo

echo "=== Test Complete ==="
echo "Check the log files in /tmp/pxpoint-logs/ to see the correlation:"
echo
ls -la /tmp/pxpoint-logs/pxpoint-ParcelProcessor-*.log | tail -5
echo
echo "The correlation IDs should show the same pipeline ID across all processes"
echo "but different process IDs for each execution."
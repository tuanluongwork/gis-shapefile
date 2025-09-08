#!/bin/bash

# Generic Log-Services Multi-Process Correlation Test Script
# This demonstrates how the new generic log-services correlation works across processes

echo "=== Generic Log-Services Multi-Process Correlation Test ==="
echo "Testing the new generic logging library with cross-process correlation"
echo

# Navigate to build directory
BUILD_DIR="$(pwd)/cpp/build"
if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory not found. Please run: cd cpp && mkdir build && cd build && cmake .. && make"
    exit 1
fi

cd "$BUILD_DIR"

# Clear any existing pipeline environment
unset LOG_PIPELINE_ID
unset LOG_PROCESS_ID

# Create log directories
mkdir -p /tmp/pxpoint-logs

echo "Building essential targets..."
if ! make -j$(nproc) dummy_parcel_processor orchestrator_process worker_process > /dev/null 2>&1; then
    echo "Build failed. Please check build configuration."
    exit 1
fi

echo "Testing C# Orchestrator -> C++ Worker pipeline..."
echo

echo "1. Starting C# Orchestrator (DummyParcelBuilderNew)..."
echo "   This will create the pipeline correlation ID and spawn C++ workers"
echo

if command -v dotnet >/dev/null 2>&1; then
    dotnet run --project ../../cs/DummyParcelBuilderNew.csproj --no-build --verbosity quiet
else
    echo "   .NET not available, running C++ processes directly..."
    echo
    
    # Fallback: run C++ processes directly to demonstrate correlation
    echo "2. Starting first C++ process (ParcelProcessor for FIPS 01001)..."
    echo "   This will create the pipeline correlation ID"
    echo
    ./bin/dummy_parcel_processor 01001
    echo
    echo "   Pipeline environment after first process:"
    echo "   LOG_PIPELINE_ID: $LOG_PIPELINE_ID"
    echo "   LOG_PROCESS_ID: $LOG_PROCESS_ID"
    echo

    echo "3. Starting second C++ process (ParcelProcessor for FIPS 01002)..."
    echo "   This should inherit the pipeline correlation ID"
    echo
    ./bin/dummy_parcel_processor 01002
    echo

    echo "4. Starting third C++ process (ParcelProcessor for FIPS 01003)..."
    echo "   This should also inherit the same pipeline correlation ID"
    echo
    ./bin/dummy_parcel_processor 01003
    echo
fi

echo "=== Test Complete ==="
echo

# Analyze log files
echo "Analyzing generated log files..."

# Check for C# logs
if ls /tmp/pxpoint-logs/orchestrator-*.log >/dev/null 2>&1; then
    echo "✅ C# Orchestrator logs found:"
    ls -la /tmp/pxpoint-logs/orchestrator-*.log | head -3
    echo
fi

# Check for C++ logs
if ls /tmp/pxpoint-logs/*ParcelProcessor*.log >/dev/null 2>&1; then
    echo "✅ C++ Worker logs found:"
    ls -la /tmp/pxpoint-logs/*ParcelProcessor*.log | head -5
    echo
fi

# Analyze correlation consistency
echo "Checking correlation consistency across processes..."

PIPELINE_IDS=$(find /tmp/pxpoint-logs -name "*.log" -exec grep -h "pipeline_id" {} \; 2>/dev/null | grep -o 'pipeline[^",:}]*' | sort -u | wc -l)

if [ "$PIPELINE_IDS" -gt 0 ]; then
    if [ "$PIPELINE_IDS" -eq 1 ]; then
        echo "✅ Correlation consistency: All processes used the same pipeline ID"
        PIPELINE_ID=$(find /tmp/pxpoint-logs -name "*.log" -exec grep -h "pipeline_id" {} \; 2>/dev/null | grep -o 'pipeline[^",:}]*' | sort -u | head -1)
        echo "   Pipeline ID: $PIPELINE_ID"
    else
        echo "⚠️  Warning: Found $PIPELINE_IDS different pipeline IDs (expected 1)"
    fi
else
    echo "⚠️  No correlation data found in logs"
fi

echo
echo "Sample log entries from different processes:"
echo "--- C# Orchestrator (if available) ---"
if ls /tmp/pxpoint-logs/orchestrator-*.log >/dev/null 2>&1; then
    head -2 /tmp/pxpoint-logs/orchestrator-*.log 2>/dev/null | head -2
fi

echo
echo "--- C++ Workers ---"
if ls /tmp/pxpoint-logs/*ParcelProcessor*.log >/dev/null 2>&1; then
    find /tmp/pxpoint-logs -name "*ParcelProcessor*.log" -exec head -1 {} \; 2>/dev/null | head -3
fi

echo
echo "=== Performance Summary ==="

# Count total log entries
TOTAL_ENTRIES=$(find /tmp/pxpoint-logs -name "*.log" -exec wc -l {} \; 2>/dev/null | awk '{sum += $1} END {print sum}')
echo "Total log entries generated: ${TOTAL_ENTRIES:-0}"

# Calculate log file sizes
TOTAL_SIZE=$(find /tmp/pxpoint-logs -name "*.log" -exec du -b {} \; 2>/dev/null | awk '{sum += $1} END {print sum}')
if [ -n "$TOTAL_SIZE" ]; then
    TOTAL_SIZE_MB=$((TOTAL_SIZE / 1024 / 1024))
    echo "Total log size: ${TOTAL_SIZE} bytes (~${TOTAL_SIZE_MB} MB)"
fi

echo
echo "=== Next Steps ==="
echo "1. Review log files in /tmp/pxpoint-logs/ directory"
echo "2. Test with ELK stack integration if available"  
echo "3. Customize YAML configuration in log-services/config/"
echo "4. Run performance benchmarks with log-services/examples/performance_example"

echo
echo "Log files location: /tmp/pxpoint-logs/"
ls -la /tmp/pxpoint-logs/ 2>/dev/null | grep -E '\.(log)$' | head -10
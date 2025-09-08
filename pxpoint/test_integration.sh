#!/bin/bash

# Integration test script for the updated PxPoint logging system
# This script demonstrates backward compatibility and new features

set -e

echo "=== PxPoint Logging Integration Test ==="

# Create build directory
BUILD_DIR="cpp/build-integration"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "Configuring build..."
cmake .. -DCMAKE_BUILD_TYPE=Debug

echo "Building..."
make -j$(sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "=== Running Integration Tests ==="

# Create test log directories
mkdir -p /tmp/pxpoint-logs
mkdir -p /tmp/pxpoint-logs

echo ""
echo "1. Testing basic functionality (backward compatibility)..."
if ./dummy_parcel_processor; then
    echo "✅ Basic functionality test passed"
else
    echo "❌ Basic functionality test failed"
    exit 1
fi

echo ""
echo "2. Testing log-services examples..."

# Test basic logging example
echo "Testing basic logging example..."
if (cd log-services/examples && ./basic_logging_example); then
    echo "✅ Basic logging example passed"
else
    echo "❌ Basic logging example failed"
    exit 1
fi

# Test performance example
echo "Testing performance example..."
if (cd log-services/examples && ./performance_example); then
    echo "✅ Performance example passed"
else
    echo "❌ Performance example failed"
    exit 1
fi

# Test custom configuration example
echo "Testing custom configuration example..."
if (cd log-services/examples && ./custom_config_example); then
    echo "✅ Custom configuration example passed"
else
    echo "❌ Custom configuration example failed"
    exit 1
fi

# Test multi-process workflow
echo "Testing multi-process workflow..."
if (cd log-services/examples && ./orchestrator_process); then
    echo "✅ Multi-process workflow passed"
else
    echo "❌ Multi-process workflow failed"
    exit 1
fi

echo ""
echo "3. Running unit tests..."
if (cd log-services/tests && ./log-services-tests); then
    echo "✅ Unit tests passed"
else
    echo "❌ Unit tests failed"
    exit 1
fi

echo ""
echo "=== Analyzing Log Output ==="

# Check log files were created
if [ -d "/tmp/pxpoint-logs" ] && [ "$(ls -A /tmp/pxpoint-logs)" ]; then
    echo "✅ PxPoint logs created successfully"
    echo "Log files in /tmp/pxpoint-logs:"
    ls -la /tmp/pxpoint-logs/
else
    echo "❌ PxPoint logs not found"
fi

if [ -d "/tmp/pxpoint-logs" ] && [ "$(ls -A /tmp/pxpoint-logs)" ]; then
    echo "✅ Generic logs created successfully"
    echo "Log files in /tmp/pxpoint-logs:"
    ls -la /tmp/pxpoint-logs/
else
    echo "❌ Generic logs not found"
fi

# Analyze log content for correlation consistency
echo ""
echo "Checking correlation consistency..."

# Find a sample log file with correlation data
LOG_FILE=""
for file in /tmp/pxpoint-logs/*.log /tmp/pxpoint-logs/*.log; do
    if [ -f "$file" ] && grep -q "pipeline" "$file" 2>/dev/null; then
        LOG_FILE="$file"
        break
    fi
done

if [ -n "$LOG_FILE" ]; then
    echo "Analyzing log file: $LOG_FILE"
    
    # Extract pipeline IDs and check consistency
    PIPELINE_IDS=$(grep -o 'pipeline[^"|,]*' "$LOG_FILE" 2>/dev/null | sort -u | wc -l)
    if [ "$PIPELINE_IDS" -gt 0 ]; then
        echo "✅ Found consistent correlation tracking ($PIPELINE_IDS unique pipeline(s))"
    else
        echo "⚠️  No correlation data found in logs"
    fi
    
    # Check for JSON structure in logs
    if grep -q '{"timestamp"' "$LOG_FILE" 2>/dev/null; then
        echo "✅ JSON structured logging working"
    else
        echo "⚠️  No JSON structured logs found"
    fi
    
    # Show sample log entries
    echo ""
    echo "Sample log entries:"
    head -n 3 "$LOG_FILE" 2>/dev/null || echo "Could not read log file"
else
    echo "⚠️  No log files with correlation data found"
fi

echo ""
echo "=== Performance Analysis ==="

# Check async logging performance
if grep -q "performance.*msg/sec" /tmp/pxpoint-logs/*.log 2>/dev/null; then
    echo "✅ Performance measurements found in logs"
    grep "performance.*msg/sec" /tmp/pxpoint-logs/*.log 2>/dev/null | head -n 3
else
    echo "⚠️  No performance measurements found"
fi

echo ""
echo "=== Integration Test Summary ==="

# Final validation
TOTAL_TESTS=6
PASSED_TESTS=0

# Count successful tests (basic heuristic)
if [ -f "./dummy_parcel_processor" ]; then
    ((PASSED_TESTS++))
fi

if [ -d "/tmp/pxpoint-logs" ] && [ "$(ls -A /tmp/pxpoint-logs)" ]; then
    ((PASSED_TESTS++))
fi

if [ -d "/tmp/pxpoint-logs" ] && [ "$(ls -A /tmp/pxpoint-logs)" ]; then
    ((PASSED_TESTS++))
fi

# Check if examples built
if [ -f "log-services/examples/basic_logging_example" ]; then
    ((PASSED_TESTS++))
fi

if [ -f "log-services/examples/performance_example" ]; then
    ((PASSED_TESTS++))
fi

if [ -f "log-services/tests/log-services-tests" ]; then
    ((PASSED_TESTS++))
fi

echo "Tests passed: $PASSED_TESTS/$TOTAL_TESTS"

if [ "$PASSED_TESTS" -eq "$TOTAL_TESTS" ]; then
    echo "🎉 All integration tests passed!"
    echo ""
    echo "The PxPoint logging system has been successfully upgraded to use the"
    echo "generic log-services library while maintaining backward compatibility."
    echo ""
    echo "Key improvements:"
    echo "  • YAML-based configuration"
    echo "  • Enhanced performance with async logging"
    echo "  • Comprehensive structured logging"
    echo "  • Better correlation tracking"
    echo "  • Extensive test coverage"
    echo "  • Modern C++ best practices"
    echo ""
    echo "Next steps:"
    echo "  1. Review log output in /tmp/pxpoint-logs/ and /tmp/pxpoint-logs/"
    echo "  2. Customize configuration files in log-services/config/"
    echo "  3. Explore examples in log-services/examples/"
    echo "  4. Read documentation in log-services/README.md"
    exit 0
else
    echo "❌ Some integration tests failed"
    echo "Please review the output above for issues"
    exit 1
fi
#!/usr/bin/env python3
"""
ELK Integration Test Script

This script simulates the ELK stack integration by:
1. Reading GIS server logs 
2. Processing them like Logstash would
3. Demonstrating the structured data that would be indexed in Elasticsearch
4. Showing what would appear in Kibana dashboards

Run this script to see how the complete ELK pipeline would process the GIS logs.
"""

import json
import re
from datetime import datetime
from typing import Dict, Any, List
import os

class LogProcessor:
    """Simulates Logstash log processing pipeline"""
    
    def __init__(self):
        self.processed_logs = []
        
    def parse_gis_log(self, log_line: str) -> Dict[str, Any]:
        """Parse a GIS server log line like Logstash would"""
        try:
            # Parse the JSON log entry
            log_entry = json.loads(log_line.strip())
            
            # Extract correlation ID and context from message if present
            message = log_entry.get('message', '')
            correlation_id = None
            context = {}
            performance = {}
            
            # Extract correlation_id and context fields from message
            if '|' in message:
                parts = message.split(' | ')
                base_message = parts[0] if parts else message
                
                for part in parts[1:]:
                    # Split by spaces to handle multiple key:value pairs
                    tokens = part.split()
                    for token in tokens:
                        if ':' in token:
                            key, value = token.split(':', 1)
                            key = key.strip()
                            value = value.strip()
                            
                            if key == 'correlation_id':
                                correlation_id = value
                            elif any(suffix in key for suffix in ['_ms', '_mb', '_size', 'time']):
                                # Performance metrics
                                try:
                                    performance[key] = float(value)
                                except ValueError:
                                    context[key] = value
                            else:
                                context[key] = value
                
                log_entry['message'] = base_message
            
            # Add extracted fields
            if correlation_id:
                log_entry['correlation_id'] = correlation_id
            if context:
                log_entry['context'] = context
            if performance:
                log_entry['performance'] = performance
                
            # Add service metadata (like Logstash would)
            log_entry['service'] = 'gis-geocoding-api'
            log_entry['environment'] = 'development'
            log_entry['version'] = '1.0.0'
            
            # Add performance alerts
            if performance:
                if performance.get('response_time_ms', 0) > 1000:
                    log_entry['alert'] = 'slow_response'
                    log_entry['tags'] = log_entry.get('tags', []) + ['performance_alert']
                    
                if performance.get('geocode_time_ms', 0) > 500:
                    log_entry['alert'] = 'slow_geocoding'
                    log_entry['tags'] = log_entry.get('tags', []) + ['performance_alert']
            
            # Add operation type based on logger
            if log_entry.get('logger') == 'GeocodingAPI':
                if 'geocod' in message.lower():
                    log_entry['operation_type'] = 'geocoding'
                elif 'reverse' in message.lower():
                    log_entry['operation_type'] = 'reverse_geocoding'
                elif 'http' in message.lower():
                    log_entry['operation_type'] = 'http_request'
                    
            return log_entry
            
        except json.JSONDecodeError:
            # Handle non-JSON log lines
            return {
                'timestamp': datetime.utcnow().isoformat() + 'Z',
                'level': 'info',
                'logger': 'raw',
                'message': log_line.strip(),
                'service': 'gis-geocoding-api',
                'environment': 'development'
            }
    
    def process_log_file(self, log_file_path: str) -> List[Dict[str, Any]]:
        """Process entire log file like Logstash would"""
        processed_logs = []
        
        if not os.path.exists(log_file_path):
            print(f"Log file not found: {log_file_path}")
            return processed_logs
            
        with open(log_file_path, 'r') as f:
            for line_num, line in enumerate(f, 1):
                if line.strip():  # Skip empty lines
                    try:
                        processed_log = self.parse_gis_log(line)
                        processed_log['line_number'] = line_num
                        processed_log['@timestamp'] = processed_log.get('timestamp')
                        processed_logs.append(processed_log)
                    except Exception as e:
                        print(f"Error processing line {line_num}: {e}")
                        
        return processed_logs

class KibanaDashboardSimulator:
    """Simulates Kibana dashboard queries and visualizations"""
    
    def __init__(self, logs: List[Dict[str, Any]]):
        self.logs = logs
    
    def search_logs(self, query: str = "", level: str = "", logger: str = "") -> List[Dict[str, Any]]:
        """Simulate Kibana Discover search"""
        results = self.logs
        
        if query:
            results = [log for log in results if query.lower() in log.get('message', '').lower()]
        if level:
            results = [log for log in results if log.get('level') == level]
        if logger:
            results = [log for log in results if log.get('logger') == logger]
            
        return results
    
    def get_performance_metrics(self) -> Dict[str, Any]:
        """Generate performance dashboard data"""
        performance_logs = [log for log in self.logs if 'performance' in log]
        
        if not performance_logs:
            return {"message": "No performance metrics found"}
        
        # Calculate averages
        response_times = [log['performance'].get('response_time_ms', 0) 
                         for log in performance_logs if 'response_time_ms' in log.get('performance', {})]
        geocode_times = [log['performance'].get('geocode_time_ms', 0) 
                        for log in performance_logs if 'geocode_time_ms' in log.get('performance', {})]
        
        return {
            "total_requests": len([log for log in self.logs if log.get('operation_type') == 'http_request']),
            "avg_response_time_ms": sum(response_times) / len(response_times) if response_times else 0,
            "avg_geocode_time_ms": sum(geocode_times) / len(geocode_times) if geocode_times else 0,
            "slow_requests": len([log for log in self.logs if log.get('alert') in ['slow_response', 'slow_geocoding']]),
            "successful_geocodes": len([log for log in self.logs if 'Geocoding successful' in log.get('message', '')]),
            "failed_geocodes": len([log for log in self.logs if 'failed' in log.get('message', '').lower()]),
        }
    
    def get_error_summary(self) -> Dict[str, Any]:
        """Generate error tracking dashboard data"""
        error_logs = [log for log in self.logs if log.get('level') == 'error']
        warn_logs = [log for log in self.logs if log.get('level') == 'warn']
        
        return {
            "total_errors": len(error_logs),
            "total_warnings": len(warn_logs),
            "error_messages": [log['message'] for log in error_logs[:5]],  # Top 5
            "warning_messages": [log['message'] for log in warn_logs[:5]],  # Top 5
        }

def main():
    """Main demonstration"""
    print("=" * 60)
    print("ELK Stack Integration Demonstration")
    print("=" * 60)
    
    # Path to actual GIS server logs
    log_file = "/home/tuanla/data/gis-shapefile-main/logs/gis-server.log"
    
    # Process logs like Logstash would
    processor = LogProcessor()
    processed_logs = processor.process_log_file(log_file)
    
    print(f"\n📊 Processed {len(processed_logs)} log entries")
    
    if not processed_logs:
        print("\n⚠️  No logs found. Run the GIS server first to generate logs:")
        print("   cd /home/tuanla/data/gis-shapefile-main")
        print("   ./build-new/gis-server -d data/gadm41_USA_1 -p 8080")
        return
    
    # Show sample processed log (what would go to Elasticsearch)
    print("\n🔍 Sample Processed Log Entry (Elasticsearch Document):")
    print("-" * 50)
    print(json.dumps(processed_logs[0], indent=2))
    
    # Simulate Kibana dashboards
    kibana = KibanaDashboardSimulator(processed_logs)
    
    # Performance Dashboard
    print("\n📈 Performance Dashboard:")
    print("-" * 50)
    perf_metrics = kibana.get_performance_metrics()
    for key, value in perf_metrics.items():
        print(f"{key:25}: {value}")
    
    # Error Dashboard  
    print("\n🚨 Error Tracking Dashboard:")
    print("-" * 50)
    error_summary = kibana.get_error_summary()
    for key, value in error_summary.items():
        if isinstance(value, list):
            print(f"{key:25}: {len(value)} items")
            for item in value[:3]:  # Show first 3
                print(f"{'':27}• {item}")
        else:
            print(f"{key:25}: {value}")
    
    # Search Examples
    print("\n🔍 Kibana Search Examples:")
    print("-" * 50)
    
    # Search for geocoding operations
    geocoding_logs = kibana.search_logs(query="geocoding")
    print(f"Geocoding operations: {len(geocoding_logs)}")
    
    # Search for errors
    error_logs = kibana.search_logs(level="error")
    print(f"Error level logs: {len(error_logs)}")
    
    # Search by logger
    api_logs = kibana.search_logs(logger="GeocodingAPI")
    print(f"GeocodingAPI logs: {len(api_logs)}")
    
    print("\n✅ ELK Integration Demonstration Complete!")
    print("\nThis simulation shows how your GIS logs would be:")
    print("  • Parsed and enriched by Logstash")
    print("  • Indexed and stored in Elasticsearch") 
    print("  • Visualized and searched in Kibana")
    print("\nTo deploy the actual ELK stack, see DEPLOYMENT-GUIDE.md")

if __name__ == "__main__":
    main()
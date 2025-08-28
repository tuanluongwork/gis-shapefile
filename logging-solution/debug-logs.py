#!/usr/bin/env python3
import json
import os

def debug_log_parsing():
    log_file = "/home/tuanla/data/gis-shapefile-main/logs/gis-server.log"
    
    if not os.path.exists(log_file):
        print(f"Log file not found: {log_file}")
        return
        
    with open(log_file, 'r') as f:
        lines = f.readlines()
    
    print(f"Found {len(lines)} lines in log file")
    
    # Look for lines with performance metrics
    for i, line in enumerate(lines):
        if any(metric in line for metric in ['_ms:', 'load_time', 'response_time', 'geocode_time']):
            print(f"\nLine {i+1} (performance metric):")
            print(line.strip())
            
            # Try to parse it
            try:
                log_entry = json.loads(line.strip())
                message = log_entry.get('message', '')
                print(f"Message: {message}")
                
                if '|' in message:
                    parts = message.split(' | ')
                    print(f"Parts: {parts}")
                    
                    for part in parts[1:]:
                        if ':' in part:
                            key, value = part.split(':', 1)
                            print(f"  Key: '{key.strip()}', Value: '{value.strip()}'")
                            
            except Exception as e:
                print(f"Error parsing: {e}")

if __name__ == "__main__":
    debug_log_parsing()
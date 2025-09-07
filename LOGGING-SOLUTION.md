# PxPoint Logging Infrastructure Architecture Design

## Architecture Overview

Based on the PxPoint system analysis, this design addresses the specific constraints and requirements of a quarterly-batch processing pipeline with mixed technology stack and limited operational resources.

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                           PxPoint Logging Architecture                          │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────────────────┐ │
│  │   Application   │    │   Application   │    │    Legacy Applications      │ │
│  │   Layer (C#)    │    │   Layer (C++)   │    │    (Existing Logging)       │ │
│  │                 │    │                 │    │                             │ │
│  │ ┌─────────────┐ │    │ ┌─────────────┐ │    │ ┌─────────────────────────┐ │ │
│  │ │   Serilog   │ │    │ │   spdlog    │ │    │ │   File Outputs          │ │ │
│  │ │   (JSON)    │ │    │ │   (JSON)    │ │    │ │   (Text/Console)        │ │ │
│  │ └─────────────┘ │    │ └─────────────┘ │    │ └─────────────────────────┘ │ │
│  └─────────────────┘    └─────────────────┘    └─────────────────────────────┘ │
│           │                       │                         │                   │
│           ▼                       ▼                         ▼                   │
├─────────────────────────────────────────────────────────────────────────────────┤
│                              Collection Layer                                   │
│                                                                                 │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────────────────┐ │
│  │   Direct TCP    │    │   File-based    │    │    Transformation           │ │
│  │   Shipping      │    │   Collection    │    │    Pipeline                 │ │
│  │                 │    │                 │    │                             │ │
│  │ ┌─────────────┐ │    │ ┌─────────────┐ │    │ ┌─────────────────────────┐ │ │
│  │ │  Logstash   │ │    │ │  Filebeat   │ │    │ │  Logstash Filters       │ │ │
│  │ │  TCP Input  │ │    │ │             │ │    │ │  - Parse Legacy Logs    │ │ │
│  │ └─────────────┘ │    │ └─────────────┘ │    │ │  - Add Correlation IDs  │ │ │
│  └─────────────────┘    └─────────────────┘    │ │  - Enrich Metadata      │ │ │
│                                                 │ └─────────────────────────┘ │ │
│                                                 └─────────────────────────────┘ │
├─────────────────────────────────────────────────────────────────────────────────┤
│                             Processing Layer                                    │
│                                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────────────┐ │
│  │                            Message Queue                                    │ │
│  │  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────────────┐ │ │
│  │  │   Redis Queue   │    │   Dead Letter   │    │    Backpressure         │ │ │
│  │  │   (Primary)     │    │   Queue         │    │    Management           │ │ │
│  │  └─────────────────┘    └─────────────────┘    └─────────────────────────┘ │ │
│  └─────────────────────────────────────────────────────────────────────────────┘ │
├─────────────────────────────────────────────────────────────────────────────────┤
│                              Storage Layer                                      │
│                                                                                 │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────────────────┐ │
│  │  Elasticsearch  │    │   Index         │    │    Data Retention           │ │
│  │  Cluster        │    │   Management    │    │    Management               │ │
│  │                 │    │                 │    │                             │ │
│  │ ┌─────────────┐ │    │ ┌─────────────┐ │    │ ┌─────────────────────────┐ │ │
│  │ │   Hot Tier  │ │    │ │ Time-based  │ │    │ │  Automated Lifecycle    │ │ │
│  │ │   (7 days)  │ │    │ │ Indices     │ │    │ │  Hot → Warm → Cold      │ │ │
│  │ │             │ │    │ │ per Build   │ │    │ │  → Delete               │ │ │
│  │ └─────────────┘ │    │ └─────────────┘ │    │ └─────────────────────────┘ │ │
│  │ ┌─────────────┐ │    │                 │    │                             │ │
│  │ │  Warm Tier  │ │    │                 │    │                             │ │
│  │ │  (30 days)  │ │    │                 │    │                             │ │
│  │ └─────────────┘ │    │                 │    │                             │ │
│  │ ┌─────────────┐ │    │                 │    │                             │ │
│  │ │  Cold Tier  │ │    │                 │    │                             │ │
│  │ │  (90 days)  │ │    │                 │    │                             │ │
│  │ └─────────────┘ │    │                 │    │                             │ │
│  └─────────────────┘    └─────────────────┘    └─────────────────────────────┘ │
├─────────────────────────────────────────────────────────────────────────────────┤
│                           Visualization Layer                                   │
│                                                                                 │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────────────────┐ │
│  │    Kibana       │    │   Dashboards    │    │    Alerting                 │ │
│  │                 │    │                 │    │                             │ │
│  │ ┌─────────────┐ │    │ ┌─────────────┐ │    │ ┌─────────────────────────┐ │ │
│  │ │  Pipeline   │ │    │ │ Operational │ │    │ │  Slack Notifications    │ │ │
│  │ │  Monitoring │ │    │ │ Overview    │ │    │ │  - Critical Errors      │ │ │
│  │ └─────────────┘ │    │ └─────────────┘ │    │ │  - Build Status         │ │ │
│  │ ┌─────────────┐ │    │ ┌─────────────┐ │    │ │  - Performance Issues   │ │ │
│  │ │  Error      │ │    │ │ Performance │ │    │ └─────────────────────────┘ │ │
│  │ │  Analysis   │ │    │ │ Analysis    │ │    │                             │ │
│  │ └─────────────┘ │    │ └─────────────┘ │    │                             │ │
│  │ ┌─────────────┐ │    │ ┌─────────────┐ │    │                             │ │
│  │ │  Data       │ │    │ │ Historical  │ │    │                             │ │
│  │ │  Quality    │ │    │ │ Comparison  │ │    │                             │ │
│  │ └─────────────┘ │    │ └─────────────┘ │    │                             │ │
│  └─────────────────┘    └─────────────────┘    └─────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────────────┘
```

## Component Architecture

### 1. Application Layer Design

**Mixed Technology Integration**
```yaml
# C# Applications (ParcelBuilderNew, etc.)
logging:
  framework: Serilog
  output: JSON
  transports:
    - type: TCP
      destination: logstash:5044
    - type: File
      path: /logs/app/{date}.log
      rollover: daily
  
# C++ Applications (BuildRtx, PromoteParcels, etc.)  
logging:
  framework: spdlog
  output: JSON
  appenders:
    - type: TCP
      destination: logstash:5045
    - type: rotating_file
      path: /logs/cpp/{app}_{date}.log
      max_size: 100MB
```

**Correlation ID Implementation**
```cpp
// C++ correlation context
class CorrelationContext {
    private:
        std::string correlation_id_;
        std::string fips_code_;
        std::string job_id_;
    
    public:
        void set_context(const std::string& corr_id, 
                        const std::string& fips,
                        const std::string& job) {
            correlation_id_ = corr_id;
            fips_code_ = fips;
            job_id_ = job;
            
            spdlog::set_default_logger(
                spdlog::default_logger()->clone(
                    fmt::format("{}_{}", correlation_id_, fips_code_)
                )
            );
        }
};

// Usage in pipeline
void process_fips(const std::string& fips) {
    auto corr_id = generate_uuid();
    CorrelationContext::instance().set_context(corr_id, fips, current_job_id);
    
    spdlog::info("Starting FIPS processing", 
        spdlog::arg("correlation_id", corr_id),
        spdlog::arg("fips", fips),
        spdlog::arg("step", "normalize"));
}
```

### 2. Collection Layer Architecture

**Hybrid Collection Strategy**
- **New Applications**: Direct TCP streaming to Logstash for real-time processing
- **Legacy Applications**: Filebeat collection from existing log files
- **Transformation Pipeline**: Unified processing regardless of input method

**Logstash Configuration**
```ruby
input {
  # Modern applications - direct TCP
  tcp {
    port => 5044
    codec => json
    tags => ["pxpoint", "structured", "csharp"]
  }
  
  tcp {
    port => 5045
    codec => json
    tags => ["pxpoint", "structured", "cpp"]
  }
  
  # Legacy applications - file-based
  beats {
    port => 5043
    tags => ["pxpoint", "legacy", "filebeat"]
  }
}

filter {
  # Parse legacy log formats
  if "legacy" in [tags] {
    grok {
      match => { 
        "message" => "%{TIMESTAMP_ISO8601:timestamp} %{LOGLEVEL:level} %{GREEDYDATA:message}"
      }
    }
    
    # Add synthetic correlation ID for legacy logs
    if ![correlation_id] {
      mutate {
        add_field => { 
          "correlation_id" => "%{[@metadata][beat]}_%{[fields][fips]}_%{[@timestamp]}"
        }
      }
    }
  }
  
  # Enrich all logs with environment context
  mutate {
    add_field => {
      "environment" => "${ENVIRONMENT:development}"
      "pipeline_version" => "${PIPELINE_VERSION:unknown}"
      "build_quarter" => "${BUILD_QUARTER:2024q4}"
    }
  }
  
  # Route to appropriate indices
  if [fields][component] == "normalization" {
    mutate { add_field => { "index_suffix" => "normalize" } }
  } else if [fields][component] == "preprocessing" {
    mutate { add_field => { "index_suffix" => "preprocess" } }
  } else {
    mutate { add_field => { "index_suffix" => "general" } }
  }
}

output {
  # Primary storage
  elasticsearch {
    hosts => ["elasticsearch-01:9200", "elasticsearch-02:9200", "elasticsearch-03:9200"]
    index => "pxpoint-%{[index_suffix]}-%{+YYYY.MM.dd}"
    template_name => "pxpoint"
    template_pattern => "pxpoint-*"
  }
  
  # Backup to Redis queue for resilience
  redis {
    host => "redis-01"
    port => 6379
    key => "pxpoint-logs-backup"
    data_type => "list"
    codec => json
  }
}
```

### 3. Storage Layer Design

**Elasticsearch Cluster Configuration**
```yaml
# Production cluster sizing
cluster:
  name: pxpoint-logging
  nodes:
    - master-01: 
        roles: [master, data_content]
        memory: 16GB
        storage: 500GB SSD
    - master-02:
        roles: [master, data_content] 
        memory: 16GB
        storage: 500GB SSD
    - master-03:
        roles: [master, data_content]
        memory: 16GB
        storage: 500GB SSD

# Index lifecycle management
indices:
  template_patterns: ["pxpoint-*"]
  lifecycle:
    hot_phase:
      duration: 7d
      replicas: 1
      shards: 3
    warm_phase:
      duration: 30d
      replicas: 0
      shards: 1
    cold_phase:
      duration: 90d
      replicas: 0
      shards: 1
    delete_phase:
      duration: 365d
```

**Index Strategy**
- **Time-based**: Daily indices for active builds, monthly for historical
- **Component-based**: Separate indices for preprocessing, normalization, post-processing
- **Build-based**: Quarterly build isolation for comparison and compliance

### 4. Processing Layer - Queue Management

**Redis Queue Configuration**
```yaml
redis:
  primary:
    host: redis-01.internal
    port: 6379
    memory: 8GB
    persistence: AOF
    
  backup:
    host: redis-02.internal  
    port: 6379
    replication: enabled
    
queues:
  log_ingestion:
    max_length: 100000
    timeout: 30s
  
  dead_letter:
    max_length: 10000
    retention: 24h
    
  backpressure:
    threshold: 80%
    action: throttle_inputs
```

**Backpressure Management**
```python
# Queue monitoring and backpressure logic
class QueueManager:
    def check_health(self):
        queue_depth = redis_client.llen('pxpoint-logs')
        
        if queue_depth > BACKPRESSURE_THRESHOLD:
            # Throttle log ingestion
            self.enable_sampling(sample_rate=0.5)
            
        if queue_depth > CRITICAL_THRESHOLD:
            # Alert operations team
            slack_client.send_alert(
                channel='#pxpoint-ops',
                message=f'Log queue critical: {queue_depth} items'
            )
```

### 5. Visualization Layer Architecture

**Dashboard Strategy**
```yaml
dashboards:
  operational:
    - pipeline_status:
        metrics: [active_fips, completed_fips, failed_fips]
        refresh: 30s
        alerts: enabled
        
    - error_analysis:
        views: [error_rate_by_component, top_errors, error_trends]
        filters: [fips, component, correlation_id]
        
    - performance_monitoring:
        metrics: [processing_time, memory_usage, database_performance]
        aggregations: [fips, processing_step, time_window]
  
  analytical:
    - data_quality:
        metrics: [validation_failures, schema_mismatches, missing_data]
        grouping: [data_source, fips, build_quarter]
        
    - historical_comparison:
        baseline: previous_quarter
        comparisons: [processing_time, error_rates, data_volumes]
```

**Alert Configuration**
```yaml
alerts:
  critical:
    - name: pipeline_failure
      condition: error_rate > 5% in 5m
      channels: [slack, pagerduty]
      
    - name: disk_space_critical
      condition: elasticsearch_disk_usage > 85%
      channels: [slack, email]
      
  warning:
    - name: slow_processing
      condition: avg_processing_time > baseline * 1.5
      channels: [slack]
      
    - name: queue_depth_high
      condition: log_queue_depth > 50000
      channels: [slack]
```

## Deployment Architecture

### 1. Infrastructure Layout

**Production Environment**
```
┌─────────────────────────────────────────────────────────────┐
│                     Production Network                      │
│                                                             │
│  ┌─────────────────┐    ┌─────────────────┐                │
│  │  Load Balancer  │    │  Elasticsearch  │                │
│  │  (Nginx)        │    │  Cluster        │                │
│  │  - Port 5601    │    │  - 3 nodes      │                │
│  │  - SSL Term     │    │  - 16GB each    │                │
│  └─────────────────┘    │  - 500GB each   │                │
│           │              └─────────────────┘                │
│           │                       │                         │
│  ┌─────────────────┐    ┌─────────────────┐                │
│  │     Kibana      │    │    Logstash     │                │
│  │  - 2 instances  │    │  - 2 instances  │                │
│  │  - 8GB each     │    │  - 8GB each     │                │
│  └─────────────────┘    └─────────────────┘                │
│                                   │                         │
│                         ┌─────────────────┐                │
│                         │     Redis       │                │
│                         │  - 2 instances  │                │
│                         │  - 8GB each     │                │
│                         └─────────────────┘                │
└─────────────────────────────────────────────────────────────┘
```

### 2. Network Configuration

**Security and Access Control**
```yaml
network:
  internal:
    cidr: 10.0.0.0/16
    subnets:
      - logging: 10.0.1.0/24
      - elasticsearch: 10.0.2.0/24
      - application: 10.0.3.0/24
      
  firewall_rules:
    inbound:
      - port: 5601
        source: corporate_network
        protocol: HTTPS
      - port: 5044-5045
        source: application_subnet
        protocol: TCP
        
    outbound:
      - port: 443
        destination: slack_api
        protocol: HTTPS
      - port: 9200
        destination: elasticsearch_subnet
        protocol: HTTP

security:
  authentication:
    kibana: LDAP
    elasticsearch: basic_auth
    
  encryption:
    transport: TLS_1.3
    storage: AES_256
    
  access_control:
    roles:
      - pxpoint_viewer: [read_logs, view_dashboards]
      - pxpoint_operator: [read_logs, view_dashboards, manage_alerts]
      - pxpoint_admin: [full_access]
```

### 3. Capacity Planning

**Resource Requirements**
```yaml
capacity_planning:
  log_volume:
    daily_logs: 50GB
    peak_processing: 200GB/day
    retention_total: 2TB
    
  elasticsearch:
    storage_growth: 15GB/day average
    index_size: ~500MB per FIPS
    query_load: 100 req/min peak
    
  network:
    log_shipping: 10Mbps sustained
    dashboard_access: 5Mbps
    replication_traffic: 20Mbps
    
  compute:
    logstash_cpu: 4 cores per instance
    elasticsearch_memory: 50% of system RAM
    kibana_memory: 2GB minimum
```

## Implementation Phases

### Phase 1: Foundation (Weeks 1-8)
- Deploy Elasticsearch cluster with basic configuration
- Implement standardized logging in 3 critical applications
- Set up basic Logstash pipeline for structured log ingestion
- Create operational dashboard for pipeline monitoring

### Phase 2: Integration (Weeks 9-16)
- Roll out logging framework to all applications
- Implement correlation ID system across pipeline
- Add legacy log transformation capabilities
- Deploy alerting for critical system events

### Phase 3: Enhancement (Weeks 17-24)
- Add advanced analytics and data quality monitoring
- Implement automated retention and archival policies
- Deploy performance optimization features
- Create historical comparison capabilities

## Operational Procedures

### 1. Monitoring and Maintenance
```bash
# Daily health checks
#!/bin/bash
echo "Checking Elasticsearch cluster health..."
curl -s "http://elasticsearch-01:9200/_cluster/health?pretty"

echo "Checking log ingestion rates..."
curl -s "http://elasticsearch-01:9200/_stats/indexing?pretty" | jq '.indices | to_entries[] | {name: .key, docs: .value.total.indexing.index_total}'

echo "Checking queue depths..."
redis-cli -h redis-01 LLEN pxpoint-logs

echo "Checking disk space..."
df -h /data/elasticsearch/
```

### 2. Backup and Recovery
```yaml
backup_strategy:
  elasticsearch_snapshots:
    frequency: daily
    retention: 30_days
    repository: s3://pxpoint-backups/elasticsearch
    
  configuration_backup:
    frequency: on_change
    location: git_repository
    includes: [logstash_configs, kibana_objects, alert_rules]
    
  disaster_recovery:
    rto: 4_hours
    rpo: 1_hour
    procedures: documented_runbook
```

This architecture balances the specific requirements of the PxPoint system - quarterly processing cycles, mixed technology stack, and limited operational resources - with enterprise logging best practices. The design provides observability foundation for the modernization effort while remaining operationally sustainable.
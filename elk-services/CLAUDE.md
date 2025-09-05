# CLAUDE.md Summary of the ELK Stack Solution

This document provides a detailed summary of the ELK (Elasticsearch, Logstash, Kibana) stack solution in the `elk-services` directory.

### 1. Overview

The `elk-services` directory contains a complete, containerized ELK stack for centralized log management and analysis. It is designed to be generic and highly configurable, allowing it to be adapted for various applications and log formats. The stack uses Docker Compose for easy orchestration of the different services.

### 2. Core Components

The solution is comprised of the following services, as defined in `docker-compose.yml`:

*   **Elasticsearch**:
    *   **Image**: `docker.elastic.co/elasticsearch/elasticsearch:8.11.0`
    *   **Purpose**: The core of the stack, used for storing, indexing, and searching the logs.
    *   **Configuration**:
        *   Runs as a single-node cluster (`discovery.type=single-node`).
        *   Security features are disabled for ease of use (`xpack.security.enabled=false`).
        *   Heap size is configurable via the `ELASTICSEARCH_HEAP_SIZE` environment variable.
        *   Exposes ports `9200` (for REST API) and `9300` (for internal cluster communication).
        *   Persists data to a Docker volume named `elasticsearch_data`.

*   **Logstash**:
    *   **Image**: `docker.elastic.co/logstash/logstash:8.11.0`
    *   **Purpose**: A data processing pipeline that ingests logs from various sources, transforms them, and sends them to Elasticsearch.
    *   **Configuration**:
        *   The pipeline configuration is mounted from `./config/logstash/pipeline`.
        *   It can ingest logs directly from a mounted log folder (`LOG_SOURCE_PATH`) or via Beats input on port `5044`.
        *   Highly configurable through environment variables like `LOG_FORMAT`, `INDEX_PREFIX`, `CUSTOM_GROK_PATTERN`, etc.
        *   Heap size is configurable via `LOGSTASH_HEAP_SIZE`.

*   **Kibana**:
    *   **Image**: `docker.elastic.co/kibana/kibana:8.11.0`
    *   **Purpose**: The visualization layer for the ELK stack. It provides a web-based interface to search, analyze, and visualize the log data stored in Elasticsearch.
    *   **Configuration**:
        *   Connects to the Elasticsearch instance.
        *   Exposes port `5601` for the web interface.

*   **Filebeat**:
    *   **Image**: `docker.elastic.co/beats/filebeat:8.11.0`
    *   **Purpose**: A lightweight log shipper that can be used to forward logs from files or other sources to Logstash or Elasticsearch.
    *   **Configuration**:
        *   Configured via `filebeat.yml`.
        *   Mounts the application's log directory (`LOG_SOURCE_PATH`) to collect logs.
        *   It also has access to the Docker socket (`/var/run/docker.sock`) and container logs, allowing it to collect logs from other running containers.

### 3. Log Processing and Pipelines

The Logstash configuration is split into multiple files within `config/logstash/pipeline/`, allowing for different log processing strategies.

*   **`generic-logs.conf`**:
    *   This is a flexible pipeline that can handle both `json` and plain `text` log formats, determined by the `LOG_FORMAT` environment variable.
    *   For text logs, it can use a `CUSTOM_GROK_PATTERN` for parsing, or it falls back to common patterns like `COMMONAPACHELOG` or `SYSLOG`.
    *   It extracts timestamps, log levels, and can be configured to extract correlation IDs and thread information using patterns from environment variables.
    *   It normalizes log levels (e.g., `warn` becomes `warning`).

*   **`application-logs.conf`**:
    *   This pipeline seems more tailored for structured JSON logs, potentially from a specific application.
    *   It has a specific grok pattern to extract `correlation_id` and other context data from a log message.
    *   It handles timestamp parsing and adds common fields like `application` and `log_source`.

### 4. Configuration and Customization

The entire stack is designed to be configured through environment variables defined in a `.env` file. The repository provides several templates for different use cases:

*   `.env.example`: For JSON-based logs.
*   `.env.simple`: For simple text logs.
*   `.env.apache`: For Apache access logs.

Key configurable variables include:
*   `APPLICATION_NAME`: A name for the application to logically separate containers and indices.
*   `LOG_SOURCE_PATH`: The path to the log files that need to be ingested.
*   `LOG_FORMAT`: The format of the logs (`json` or `text`).
*   `INDEX_PREFIX`: The prefix for Elasticsearch indices.
*   `CUSTOM_GROK_PATTERN`: A custom Grok pattern for parsing text logs.

### 5. How to Run

1.  Copy one of the `.env` templates to `.env`.
2.  Customize the `.env` file with the appropriate settings for the target application.
3.  Run `docker-compose up -d` to start the entire stack.
4.  Access Kibana at `http://localhost:5601` to view and analyze the logs.

In summary, this is a robust and reusable ELK stack that can be quickly deployed and configured to provide powerful log analysis capabilities for a wide range of applications.

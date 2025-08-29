# Project Overview

This project is a C++ enterprise logging solution for a GIS Geocoding API. It uses the `spdlog` library for high-performance, structured logging and the ELK stack (Elasticsearch, Logstash, Kibana) for log aggregation, processing, and visualization. Filebeat is used as a log shipper.

The core of the project is a C++ logging library that provides a simple interface for logging structured data. The library is designed to be thread-safe and to have a low performance overhead. It supports logging to multiple sinks, including a rotating file sink and a console sink.

The ELK stack is used to collect, process, and visualize the logs. Filebeat is used to ship the logs from the application server to Logstash. Logstash is used to parse the logs, enrich them with metadata, and send them to Elasticsearch. Elasticsearch is used to store and index the logs. Kibana is used to visualize the logs and create dashboards.

## Building and Running

The project uses Docker and Docker Compose to manage the ELK stack. The C++ application is built with CMake.

**Key commands:**

*   `elk-infrastructure/deploy-logging.sh`: Deploys the ELK stack using Docker Compose.
*   `make build`: Builds the C++ application.
*   `make run`: Runs the C++ application.
*   `make test-logging`: Tests the logging infrastructure.
*   `make status`: Checks the status of the Docker containers.
*   `make logs`: Tails the logs of the Docker containers.

## Development Conventions

*   **Logging:** The project uses the `spdlog` library for logging. All logs should be structured and should include a correlation ID. The `LOG_INFO`, `LOG_WARN`, `LOG_ERROR`, and `LOG_DEBUG` macros should be used for logging.
*   **Configuration:** The logging library is configured using a YAML file (`config/logging.yaml`). The ELK stack is configured using a set of YAML and configuration files in the `elk-infrastructure/elk-config` directory.
*   **Dependencies:** The project uses `spdlog` and `yaml-cpp` as C++ dependencies. These are managed using CMake's `FetchContent` module.

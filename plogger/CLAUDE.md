# Project Overview

This project is a C++ logging solution for a GIS Geocoding API. It uses the `spdlog` library for high-performance, structured logging with JSON output format.

The core of the project is a C++ logging library that provides a simple interface for logging structured data. The library is designed to be thread-safe and to have a low performance overhead. It supports logging to multiple sinks, including a rotating file sink and a console sink.

## Building and Running

The C++ application is built with CMake.

**Key commands:**

*   `make build`: Builds the C++ application.
*   `make run`: Runs the C++ application.

## Development Conventions

*   **Logging:** The project uses the `spdlog` library for logging. All logs should be structured and should include a correlation ID. The `LOG_INFO`, `LOG_WARN`, `LOG_ERROR`, and `LOG_DEBUG` macros should be used for logging.
*   **Configuration:** The logging library is configured using a YAML file (`config/logging.yaml`).
*   **Dependencies:** The project uses `spdlog` and `yaml-cpp` as C++ dependencies. These are managed using CMake's `FetchContent` module.

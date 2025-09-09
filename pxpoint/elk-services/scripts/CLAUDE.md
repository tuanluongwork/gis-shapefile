### Solution Summary: `elk-services/scripts`

This document provides a staff engineer's summary of the management scripts located in the `elk-services/scripts` directory. These scripts provide a convenient command-line interface for managing the lifecycle of the ELK stack.

#### 1. Architectural Overview & Purpose

The scripts are designed as a cohesive set of tools to simplify common Docker Compose operations. They act as a "Makefile-like" interface, abstracting away the verbosity of `docker-compose` commands and adding extra logic to ensure robust operation. Each script is self-contained but also designed to work with the others (e.g., `restart.sh` calls `stop.sh` and `deploy.sh`).

The main goals of these scripts are:
*   **Ease of Use**: Provide simple, memorable commands for common tasks (`deploy`, `stop`, `status`, etc.).
*   **Robustness**: Add checks and validations that `docker-compose` alone does not provide (e.g., waiting for services to be healthy, checking for Docker daemon availability).
*   **Consistency**: Ensure that operations are performed in a consistent and correct order.
*   **Clarity**: Provide informative output to the user about what is happening.

#### 2. Script-by-Script Breakdown

*   **`deploy.sh`**:
    *   **Purpose**: The primary script for starting and deploying the ELK stack.
    *   **Functionality**:
        1.  Checks if Docker is running and `docker-compose` is installed.
        2.  Pulls the latest Docker images for the services.
        3.  Starts all services in detached mode using `docker-compose up -d`.
        4.  **Crucially, it waits for Elasticsearch and Kibana to be healthy** by polling their respective health check endpoints. This is a key feature for ensuring the stack is fully operational before the script exits.
        5.  If a service fails to start within a timeout, it prints the service's logs to aid in debugging.
        6.  Finally, it prints the status of all containers and the URLs for accessing the services.

*   **`stop.sh`**:
    *   **Purpose**: To stop the running ELK stack.
    *   **Functionality**:
        1.  A straightforward wrapper around the `docker-compose down` command.
        2.  This command stops and removes the containers but **preserves the named volumes**, so data is not lost.

*   **`restart.sh`**:
    *   **Purpose**: To perform a clean restart of the entire stack.
    *   **Functionality**:
        1.  Orchestrates a stop and start sequence by calling `stop.sh` followed by `deploy.sh`.
        2.  Includes a `sleep 10` between stopping and starting to allow time for network resources to be fully released, preventing potential conflicts on restart.

*   **`status.sh`**:
    *   **Purpose**: To provide a comprehensive overview of the stack's health.
    *   **Functionality**:
        1.  Shows the output of `docker-compose ps` to display the state of the containers.
        2.  Performs **active health checks** by curling the API endpoints of Elasticsearch, Logstash, and Kibana to confirm they are responsive.
        3.  For Elasticsearch, it also fetches and displays the cluster health status (`green`, `yellow`, or `red`).
        4.  Provides information on Docker disk usage and lists the ELK-related volumes.

*   **`logs.sh`**:
    *   **Purpose**: To easily view the logs from the running services.
    *   **Functionality**:
        1.  If run without arguments, it tails the logs of all services combined (`docker-compose logs -f`).
        2.  If a specific service name (`elasticsearch`, `logstash`, `kibana`, or `filebeat`) is provided as an argument, it shows the logs for only that service.
        3.  Includes input validation to ensure only valid service names are accepted.

*   **`cleanup.sh`**:
    *   **Purpose**: To completely wipe the ELK stack, including all data. This is a destructive operation intended for a clean reset.
    *   **Functionality**:
        1.  Prompts the user for confirmation to prevent accidental data loss.
        2.  Runs `docker-compose down -v`, which stops the containers and **removes the associated named volumes**, thereby deleting all Elasticsearch data.
        3.  Also prunes any dangling containers to keep the Docker environment clean.

#### 3. Summary for a Staff Engineer

This collection of scripts represents a mature and thoughtful approach to managing a containerized application. It demonstrates an understanding of operational best practices that go beyond simple development.

**Strengths**:
*   **Operational Robustness**: The health checks in `deploy.sh` and `status.sh` are a key feature, moving this from a simple set of commands to a reliable management toolkit.
*   **User-Friendly Abstraction**: The scripts successfully hide the complexity of Docker Compose and provide an intuitive interface for users who may not be Docker experts.
*   **Clear Separation of Concerns**: The distinction between `stop.sh` (non-destructive) and `cleanup.sh` (destructive) is critical and well-implemented.
*   **Good Scripting Practices**: The scripts use `set -e` for safety (exiting on error), use `SCRIPT_DIR` to be runnable from any directory, and provide helpful, color-coded output.

This script suite is a valuable asset that significantly improves the developer and operator experience of the `elk-services` solution. It provides the kind of tooling that is essential for managing complex, multi-service applications efficiently.

cmake_minimum_required(VERSION 3.16)

# Basic usage example
add_executable(basic_logging_example basic_logging_example.cpp)
target_link_libraries(basic_logging_example PRIVATE log-services)

# Multi-process example
add_executable(worker_process worker_process.cpp)
target_link_libraries(worker_process PRIVATE log-services)

add_executable(orchestrator_process orchestrator_process.cpp)
target_link_libraries(orchestrator_process PRIVATE log-services)

# Performance measurement example
add_executable(performance_example performance_example.cpp)
target_link_libraries(performance_example PRIVATE log-services)

# Custom configuration example
add_executable(custom_config_example custom_config_example.cpp)
target_link_libraries(custom_config_example PRIVATE log-services)

# Copy configuration files to build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../config/logging.yaml 
               ${CMAKE_CURRENT_BINARY_DIR}/logging.yaml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../config/logging-development.yaml 
               ${CMAKE_CURRENT_BINARY_DIR}/logging-development.yaml COPYONLY)
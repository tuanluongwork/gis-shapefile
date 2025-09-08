cmake_minimum_required(VERSION 3.16)
project(log-services VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Find or fetch dependencies
find_package(PkgConfig QUIET)

# Try to find spdlog
find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
    include(FetchContent)
    message(STATUS "spdlog not found, fetching from GitHub...")
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.12.0
    )
    FetchContent_MakeAvailable(spdlog)
endif()

# Try to find yaml-cpp
find_package(yaml-cpp QUIET)
if(NOT yaml-cpp_FOUND)
    include(FetchContent)
    message(STATUS "yaml-cpp not found, fetching from GitHub...")
    FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG 0.8.0
    )
    
    # Disable yaml-cpp tests and tools
    set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(yaml-cpp)
endif()

# Define the log-services library
add_library(log-services STATIC
    src/correlation_manager.cpp
    src/structured_logger.cpp
)

# Set target properties
target_include_directories(log-services
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(log-services
    PUBLIC
        spdlog::spdlog
        yaml-cpp
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(log-services PRIVATE ws2_32 wsock32)
endif()

# Install rules (disabled for now to avoid export complexity in demo)
# include(GNUInstallDirs)

# install(TARGETS log-services
#     EXPORT log-services-targets
#     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
# )

# install(DIRECTORY include/
#     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/log-services
# )

# install(DIRECTORY config/
#     DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/log-services/config
# )

# Build examples if requested
option(BUILD_LOG_SERVICES_EXAMPLES "Build log-services examples" ON)
if(BUILD_LOG_SERVICES_EXAMPLES)
    add_subdirectory(examples)
endif()

# Build tests if requested
option(BUILD_LOG_SERVICES_TESTS "Build log-services tests" ON)
if(BUILD_LOG_SERVICES_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
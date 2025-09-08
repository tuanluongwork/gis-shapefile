cmake_minimum_required(VERSION 3.16)
project(PxPointDummy)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find spdlog
find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.12.0
    )
    FetchContent_MakeAvailable(spdlog)
endif()

# Find yaml-cpp
find_package(yaml-cpp QUIET)
if(NOT yaml-cpp_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG yaml-cpp-0.7.0
    )
    FetchContent_MakeAvailable(yaml-cpp)
endif()

# Create PxPoint correlation library
add_library(pxpoint_correlation STATIC
    pxpoint_correlation.cpp
    pxpoint_logger.cpp
)

target_include_directories(pxpoint_correlation PUBLIC .)
target_link_libraries(pxpoint_correlation 
    PUBLIC 
        spdlog::spdlog 
        yaml-cpp
)

# Create dummy parcel processor executable
add_executable(dummy_parcel_processor
    dummy_parcel_processor.cpp
)

target_link_libraries(dummy_parcel_processor
    PRIVATE 
        pxpoint_correlation
        spdlog::spdlog
        yaml-cpp
)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(dummy_parcel_processor PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set output directory
set_target_properties(dummy_parcel_processor
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Installation
install(TARGETS dummy_parcel_processor
    RUNTIME DESTINATION bin
)
cmake_minimum_required(VERSION 3.16)
project(PxPointDummy)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build the log-services library first
add_subdirectory(log-services)

# Create dummy parcel processor executable using log-services directly
add_executable(dummy_parcel_processor
    dummy_parcel_processor.cpp
)

target_link_libraries(dummy_parcel_processor
    PRIVATE 
        log-services
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
cmake_minimum_required(VERSION 3.15)
project(GISShapefileProcessor 
    VERSION 1.0.0
    DESCRIPTION "Modern C++ GIS Shapefile Processor with Geocoding"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4 /permissive-)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Find required packages
find_package(Threads REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Core library
add_library(gis-core STATIC
    src/shapefile/shapefile_reader.cpp
    src/shapefile/geometry.cpp
    src/shapefile/dbf_reader.cpp
    src/geocoding/geocoder.cpp
    src/spatial/spatial_index.cpp
)

target_include_directories(gis-core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# CLI Tools
add_executable(shp-info tools/shp_info.cpp)
target_link_libraries(shp-info gis-core Threads::Threads)

add_executable(geocoder tools/geocoder_cli.cpp)
target_link_libraries(geocoder gis-core Threads::Threads)

add_executable(spatial-query tools/spatial_query.cpp)
target_link_libraries(spatial-query gis-core Threads::Threads)

# Web API Server
add_executable(gis-server server/main.cpp server/http_server.cpp)
target_link_libraries(gis-server gis-core Threads::Threads)

# Add network libraries for Windows
if(WIN32)
    target_link_libraries(gis-server ws2_32)
endif()

# Examples
add_executable(example-basic examples/basic_usage.cpp)
target_link_libraries(example-basic gis-core)

add_executable(example-performance examples/performance_demo.cpp)
target_link_libraries(example-performance gis-core Threads::Threads)

# Testing (if Google Test is available)
find_package(GTest QUIET)
if(GTest_FOUND)
    enable_testing()
    
    add_executable(unit-tests
        tests/test_shapefile_reader.cpp
        tests/test_geocoder.cpp
        tests/test_spatial_index.cpp
    )
    
    target_link_libraries(unit-tests 
        gis-core 
        GTest::gtest 
        GTest::gtest_main
        Threads::Threads
    )
    
    add_test(NAME UnitTests COMMAND unit-tests)
endif()

# Installation
install(TARGETS gis-core shp-info geocoder spatial-query gis-server
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/ DESTINATION include)

# CPack configuration
set(CPACK_PACKAGE_NAME "GISShapefileProcessor")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
include(CPack)

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

# Web API Server
add_executable(gis-server server/main.cpp server/http_server.cpp)
target_link_libraries(gis-server gis-core Threads::Threads)

# Add network libraries for Windows
if(WIN32)
    target_link_libraries(gis-server ws2_32)
endif()

# Installation
install(TARGETS gis-core gis-server
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

cmake_minimum_required(VERSION 3.16)

# Find or fetch Catch2 for testing
find_package(Catch2 3 QUIET)
if(NOT Catch2_FOUND)
    include(FetchContent)
    message(STATUS "Catch2 not found, fetching from GitHub...")
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(Catch2)
endif()

# Test executable
add_executable(log-services-tests
    test_main.cpp
    test_correlation_manager.cpp
    test_structured_logger.cpp
    test_integration.cpp
)

target_link_libraries(log-services-tests
    PRIVATE
        log-services
        Catch2::Catch2WithMain
)

# Copy test configuration files
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../config/logging.yaml 
               ${CMAKE_CURRENT_BINARY_DIR}/test-logging.yaml COPYONLY)

# Register tests with CTest
include(CTest)
include(Catch)
catch_discover_tests(log-services-tests)
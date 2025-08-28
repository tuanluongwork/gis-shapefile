# Core library source files
target_sources(gis-core
    PRIVATE
        shapefile/geometry.cpp
        shapefile/shapefile_reader.cpp
        geocoding/geocoder.cpp
        spatial/spatial_index.cpp
)

# Include directories
target_include_directories(gis-core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

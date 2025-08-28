# Project Overview

This project is a C++ library for working with GIS (Geographic Information System) data, specifically shapefiles. It provides functionalities for reading shapefiles, parsing geographic data, and performing geocoding and spatial indexing.

The core components of the library are:

*   **Shapefile Reader:** A robust parser for `.shp`, `.shx`, and `.dbf` files, capable of reading various geometric shapes (points, polylines, polygons) and their associated attributes.
*   **Geocoder:** A geocoding engine that can convert addresses into geographic coordinates (latitude and longitude) and vice versa. It includes an address parser with support for abbreviations and fuzzy matching.
*   **Spatial Index:** A grid-based spatial index for efficient querying of geometric data. This allows for fast searching of shapes within a given area.
*   **Geometry Library:** A set of classes for representing and manipulating geometric objects such as points, bounding boxes, polylines, and polygons.

The library is written in C++ and uses CMake for building.

# Building and Running

To build the project, you will need a C++ compiler and CMake installed.

```bash
# Create a build directory
mkdir build
cd build

# Configure the project
cmake ..

# Build the project
make
```

This will compile the source code and create a static library called `libgis-core.a`.

As this is a library, there is no main executable to run. You would typically link this library into your own application to use its functionalities.

# Development Conventions

*   The code is organized into namespaces (`gis`).
*   The code follows a consistent naming convention (e.g., `ClassName`, `functionName`, `variable_name_`).
*   The code is well-commented and includes header guards.
*   The project uses C++11 features such as `std::unique_ptr` and `std::unordered_map`.
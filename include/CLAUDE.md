# Project Overview

This project is a C++ library for reading and processing GIS (Geographic Information System) shapefiles. Shapefiles are a popular geospatial vector data format for geographic information system software. This library provides functionalities for reading shapefile data, performing spatial queries, and geocoding addresses.

The library is composed of the following main components:

*   **Shapefile Reader**: A component for reading geometry and attribute data from `.shp`, `.shx`, and `.dbf` files.
*   **Geometry Library**: A set of classes representing various geometric shapes such as points, polylines, and polygons.
*   **Spatial Index**: An R-tree based spatial index for efficient querying of spatial data.
*   **Geocoder**: A tool for converting street addresses into geographic coordinates (latitude and longitude) and vice-versa.

# Building and Running

**TODO:** There are no build or run instructions in the provided files. Information about the build system (e.g., Makefile, CMakeLists.txt) is needed to provide instructions on how to build and run the project.

# Development Conventions

The code follows modern C++ conventions, including the use of smart pointers (`std::unique_ptr`) for memory management and `enum class` for strongly typed enumerations. The code is organized into a `gis` namespace. Header files are well-documented using a Doxygen-style format.

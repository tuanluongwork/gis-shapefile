# GIS Shapefile Processor & Geocoder

A modern C++ library and application suite for processing ESRI Shapefiles with geocoding capabilities.

## 🎯 Project Overview

This project showcases a production-ready C++ implementation for:
- **Shapefile Reading & Processing**: Complete ESRI Shapefile (.shp, .shx, .dbf) parser
- **Geocoding Engine**: Address-to-coordinate conversion system
- **Reverse Geocoding**: Coordinate-to-address lookup
- **Spatial Indexing**: R-tree implementation for efficient spatial queries
- **Cross-Platform Support**: Windows, Linux, and macOS compatibility

## 🔧 Building the Project

### Prerequisites

**Required:**
- **C++17 Compiler**: GCC 9+, Clang 10+, or MSVC 2019+
- **CMake**: Version 3.15 or newer
- **Git**: For cloning the repository

### Platform-Specific Setup

#### Windows (Visual Studio)
```cmd
# Install prerequisites (using vcpkg or manual installation)
# Ensure CMake and Visual Studio 2019+ are installed

# Clone and build
git clone https://github.com/tuanluongwork/gis-shapefile.git
cd gis-shapefile
mkdir build && cd build

# Configure for Visual Studio
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release

# Or for Debug builds
cmake --build . --config Debug
```

#### Linux (Ubuntu/Debian)
```bash
# Install dependencies
sudo apt-get update
sudo apt-get install build-essential cmake git libgtest-dev

# Clone and build
git clone https://github.com/tuanluongwork/gis-shapefile.git
cd gis-shapefile
mkdir build && cd build

# Configure and build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# For Debug builds
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

### Running the Applications

```bash
build\Debug\gis-server.exe --port 8080 --data data/gadm41_USA_1
```
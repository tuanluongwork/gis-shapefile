# GIS Shapefile Processor & Geocoder

A modern C++ library and web application for processing ESRI Shapefiles with geocoding capabilities.

## 1. Project Overview

This project showcases a production-ready C++ implementation for:
- **Shapefile Reading & Processing**: Complete ESRI Shapefile (.shp, .shx, .dbf) parser
- **Geocoding Engine**: Address-to-coordinate conversion system
- **Reverse Geocoding**: Coordinate-to-address lookup
- **Spatial Indexing**: R-tree implementation for efficient spatial queries
- **Cross-Platform Support**: Windows, Linux, and macOS compatibility

## 2. Building the Project

### 2.1. Prerequisites

**Required:**
- **C++17 Compiler**: GCC 9+, Clang 10+, or MSVC 2019+
- **CMake**: Version 3.15 or newer
- **Git**: For cloning the repository

### 2.2. Platform-Specific Setup

#### 2.2.1. Windows (Visual Studio)
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

#### 2.2.2. Linux (Ubuntu/Debian)
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

### 3. Running the Applications

```bash
build\Debug\gis-server.exe --port 8080 --data data/gadm41_USA_1
```

### 3. Testing the Applications

```bash
GET http://localhost:8080/health
{
  "status": "healthy",
  "data_loaded": true,
  "timestamp": "2025-07-21T14:30:46Z"
}
GET http://localhost:8080/stats
{
    "service": "GIS Geocoding API",
    "data_loaded": true,
    "geocoder_stats": Geocoder Statistics:\n  Total Records: 3148\n  Street Index Entries: 0\n  City Index Entries: 0\n  Zip Index Entries: 0\n,
    "timestamp": "2025-07-21T14:25:19Z"
}
GET http://localhost:8080/geocode?address=TEXAS
{
  "input_address": "TEXAS",
  "success": true,
  "result": {
    "latitude": 31.79466057,
    "longitude": -95.66297150,
    "matched_address": "Texas",
    "confidence": 1.000,
    "match_type": "exact"
  }
}
GET http://localhost:8080/reverse?lat=40.7128&lng=-74.0060
{
  "input_coordinates": {
    "latitude": 40.71280000,
    "longitude": -74.00600000
  },
  "success": true,
  "result": {
    "address": "New York",
    "confidence": 1.000,
    "match_type": "reverse"
  }
}
```

## 4. References
https://en.wikipedia.org/wiki/Shapefile

# GEMINI Project Context: GIS Shapefile Geocoding Server

## Project Overview

This project is a lightweight, standalone HTTP server written in C++ that provides geocoding and reverse geocoding services. It's designed to load address and location data from a shapefile and expose a simple JSON-based API for querying it.

The server is self-contained, with a basic HTTP server implementation and API logic that handles requests for geocoding (address to coordinates), reverse geocoding (coordinates to address), health checks, and statistics.

**Key Technologies:**
*   **Language:** C++
*   **Core Logic:** Custom-built HTTP server, GIS geocoding and shapefile reading logic (likely in `gis/` directory, based on includes).

## Building and Running

**TODO:** The exact build commands are not present in the directory. A `Makefile` or `CMakeLists.txt` would be beneficial. Based on the source files, a likely compilation command would be:

```bash
# Note: This is an inferred command and may need adjustment.
# It assumes the GIS library files are in a parent directory.
g++ -std=c++17 -I.. http_server.cpp main.cpp -o geocode_server -pthread
```

**Running the Server:**

Once built, the server is executed with command-line arguments to specify the port and the path to the shapefile data.

```bash
./geocode_server [options]
```

**Options:**
*   `-p, --port <port>`: Sets the server port (default: 8080).
*   `-d, --data <path>`: Path to the shapefile data required for geocoding.
*   `-h, --help`: Displays the help message.

**Example:**
```bash
./geocode_server --port 9000 --data /path/to/your/shapefile_data
```

## API Endpoints

The server provides the following RESTful API endpoints:

*   **`GET /`**
    *   **Description:** Returns a welcome message with information about the API, its version, and available endpoints.

*   **`GET /geocode?address=<address>`**
    *   **Description:** Converts a street address into geographic coordinates (latitude and longitude).
    *   **Example:** `/geocode?address=1600+Amphitheatre+Parkway,+Mountain+View,+CA`

*   **`GET /reverse?lat=<lat>&lng=<lng>`**
    *   **Description:** Converts geographic coordinates into the nearest street address.
    *   **Example:** `/reverse?lat=37.422&lng=-122.084`

*   **`GET /health`**
    *   **Description:** Performs a health check on the service, indicating if the server is running and if the shapefile data is loaded.

*   **`GET /stats`**
    *   **Description:** Provides service statistics, including information from the geocoder.

## Development Conventions

*   The code is organized within the `gis` namespace.
*   The HTTP server logic is separated into `http_server.h` and `http_server.cpp`.
*   The main application and API endpoint logic resides in `main.cpp`.
*   The server is multi-threaded, with the main server loop running in a separate thread.
*   Dependencies on platform-specific networking libraries (`winsock2` for Windows, `sys/socket` for POSIX) are handled via `#ifdef`.

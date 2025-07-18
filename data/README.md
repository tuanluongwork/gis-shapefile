# Sample Data Directory

This directory contains sample shapefiles and test data for the GIS Shapefile Processor.

## Files

### Sample Shapefiles
- `sample.shp` - Main shapefile containing geometric data
- `sample.shx` - Shapefile index
- `sample.dbf` - Attribute database file
- `sample.prj` - Projection information

### Test Data
- `addresses.csv` - Sample addresses for geocoding tests
- `poi.geojson` - Points of interest in GeoJSON format

## Data Sources

The sample data is provided for demonstration and testing purposes only. In a production environment, you would typically work with:

- **Administrative Boundaries**: Country, state, county, and city boundaries
- **Street Networks**: Road centerlines with address ranges
- **Points of Interest**: Businesses, landmarks, and facilities
- **Postal Codes**: ZIP codes and postal boundaries

## Usage in Examples

The basic usage example loads data from this directory:

```cpp
#include "gis/shapefile_reader.h"

int main() {
    gis::ShapefileReader reader;
    if (reader.open("data/sample.shp")) {
        // Process shapefile data
        auto geometries = reader.readAllGeometries();
        std::cout << "Loaded " << geometries.size() << " geometries\n";
    }
    return 0;
}
```

## Creating Your Own Test Data

To create your own test shapefiles:

1. Use QGIS, ArcGIS, or other GIS software
2. Export data in ESRI Shapefile format
3. Ensure all required files (.shp, .shx, .dbf) are present
4. Place files in this directory for testing

## Data Formats Supported

- **Shapefile**: ESRI Shapefile format (.shp, .shx, .dbf)
- **GeoJSON**: For lightweight web applications
- **CSV**: For address data and point coordinates
- **KML**: Google Earth format (planned)

For more information on working with GIS data, see the project documentation.

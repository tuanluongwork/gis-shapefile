#include "gis/shapefile_reader.h"
#include "gis/geocoder.h"
#include "gis/spatial_index.h"
#include <iostream>
#include <chrono>

/**
 * @brief Basic usage example demonstrating core GIS shapefile functionality
 * 
 * This example shows how to:
 * 1. Read shapefiles and extract geometric data
 * 2. Perform basic geocoding operations
 * 3. Use spatial indexing for efficient queries
 * 4. Work with different geometry types
 */

void demonstrateShapefileReading() {
    std::cout << "=== Shapefile Reading Demo ===\n\n";
    
    // Note: In a real scenario, you would have actual shapefile data
    // For this demo, we'll show the API usage
    
    std::cout << "1. Opening shapefile...\n";
    gis::ShapefileReader reader("data/sample");
    
    if (reader.open()) {
        std::cout << "   ✓ Shapefile opened successfully\n";
        std::cout << "   Shape Type: " << static_cast<int>(reader.getShapeType()) << "\n";
        std::cout << "   Record Count: " << reader.getRecordCount() << "\n";
        
        gis::BoundingBox bounds = reader.getBounds();
        std::cout << "   Bounds: (" << bounds.min_x << ", " << bounds.min_y 
                  << ") to (" << bounds.max_x << ", " << bounds.max_y << ")\n\n";
        
        // Read and display field information
        const auto& fields = reader.getFieldDefinitions();
        std::cout << "   Fields (" << fields.size() << "):\n";
        for (const auto& field : fields) {
            std::cout << "     - " << field.name << " (type: " 
                      << static_cast<int>(field.type) << ", length: " 
                      << static_cast<int>(field.length) << ")\n";
        }
        
        std::cout << "\n2. Reading sample records...\n";
        
        // Read first few records
        for (uint32_t i = 0; i < std::min(3u, reader.getRecordCount()); ++i) {
            auto record = reader.readRecord(i);
            if (record && record->geometry) {
                std::cout << "   Record " << i << ":\n";
                
                // Display geometry information
                gis::BoundingBox geom_bounds = record->geometry->getBounds();
                std::cout << "     Geometry bounds: (" << geom_bounds.min_x 
                          << ", " << geom_bounds.min_y << ") to (" 
                          << geom_bounds.max_x << ", " << geom_bounds.max_y << ")\n";
                
                // Display some attributes
                std::cout << "     Attributes: " << record->attributes.size() << " fields\n";
                
                // Example of accessing specific geometry types
                if (record->geometry->getType() == gis::ShapeType::Point) {
                    auto* point_geom = dynamic_cast<gis::PointGeometry*>(record->geometry.get());
                    if (point_geom) {
                        const auto& point = point_geom->getPoint();
                        std::cout << "     Point: (" << point.x << ", " << point.y << ")\n";
                    }
                } else if (record->geometry->getType() == gis::ShapeType::Polygon) {
                    auto* poly_geom = dynamic_cast<gis::PolygonGeometry*>(record->geometry.get());
                    if (poly_geom) {
                        std::cout << "     Polygon with " << poly_geom->getNumRings() << " rings\n";
                    }
                }
            }
        }
        
        reader.close();
    } else {
        std::cout << "   ⚠ Could not open shapefile (this is expected in demo mode)\n";
        std::cout << "   In real usage, provide path to actual .shp/.shx/.dbf files\n";
    }
    
    std::cout << "\n";
}

void demonstrateGeocoding() {
    std::cout << "=== Geocoding Demo ===\n\n";
    
    gis::Geocoder geocoder;
    
    std::cout << "1. Initializing geocoder...\n";
    
    // In real usage, you would load actual address data
    // geocoder.loadAddressData("data/addresses");
    
    std::cout << "   ✓ Geocoder initialized\n";
    
    std::cout << "\n2. Address parsing examples...\n";
    
    gis::AddressParser parser;
    
    std::vector<std::string> test_addresses = {
        "123 Main Street, Anytown, CA 12345",
        "456 Oak Ave, Springfield, IL 62701",
        "789 Broadway Apt 5B, New York, NY 10001"
    };
    
    for (const auto& address : test_addresses) {
        std::cout << "   Input: " << address << "\n";
        
        gis::ParsedAddress parsed = parser.parse(address);
        std::cout << "   Parsed:\n";
        std::cout << "     House Number: " << parsed.house_number << "\n";
        std::cout << "     Street: " << parsed.street_name << "\n";
        std::cout << "     Type: " << parsed.street_type << "\n";
        std::cout << "     City: " << parsed.city << "\n";
        std::cout << "     State: " << parsed.state << "\n";
        std::cout << "     Zip: " << parsed.zip_code << "\n";
        std::cout << "     Valid: " << (parsed.isValid() ? "Yes" : "No") << "\n\n";
    }
    
    std::cout << "3. Geocoding simulation...\n";
    
    // Simulate geocoding results
    for (const auto& address : test_addresses) {
        std::cout << "   Geocoding: " << address << "\n";
        
        // In real usage with loaded data, this would return actual results
        gis::GeocodeResult result = geocoder.geocode(address);
        
        if (result.confidence_score > 0.0) {
            std::cout << "     ✓ Match found with " << (result.confidence_score * 100) << "% confidence\n";
            std::cout << "     Coordinates: (" << result.coordinate.x << ", " << result.coordinate.y << ")\n";
        } else {
            std::cout << "     ⚠ No match (expected without loaded data)\n";
        }
        std::cout << "\n";
    }
}

void demonstrateSpatialOperations() {
    std::cout << "=== Spatial Operations Demo ===\n\n";
    
    std::cout << "1. Geometry operations...\n";
    
    // Create sample geometries
    gis::Point2D p1(0.0, 0.0);
    gis::Point2D p2(1.0, 1.0);
    gis::Point2D p3(2.0, 0.0);
    
    auto point_geom = std::make_unique<gis::PointGeometry>(p1);
    std::cout << "   Created point at (" << p1.x << ", " << p1.y << ")\n";
    
    // Create a simple triangle polygon
    std::vector<gis::Point2D> triangle = {p1, p2, p3, p1};  // Close the ring
    std::vector<std::vector<gis::Point2D>> rings = {triangle};
    auto polygon_geom = std::make_unique<gis::PolygonGeometry>(rings);
    
    std::cout << "   Created triangle polygon\n";
    
    // Test bounding boxes
    gis::BoundingBox point_bounds = point_geom->getBounds();
    gis::BoundingBox poly_bounds = polygon_geom->getBounds();
    
    std::cout << "   Point bounds: (" << point_bounds.min_x << ", " << point_bounds.min_y 
              << ") to (" << point_bounds.max_x << ", " << point_bounds.max_y << ")\n";
    std::cout << "   Polygon bounds: (" << poly_bounds.min_x << ", " << poly_bounds.min_y 
              << ") to (" << poly_bounds.max_x << ", " << poly_bounds.max_y << ")\n";
    
    // Test point-in-polygon
    gis::Point2D test_point(0.5, 0.3);
    bool inside = polygon_geom->contains(test_point);
    std::cout << "   Point (" << test_point.x << ", " << test_point.y 
              << ") is " << (inside ? "inside" : "outside") << " the polygon\n";
    
    std::cout << "\n2. Bounding box operations...\n";
    
    gis::BoundingBox bbox1(0.0, 0.0, 2.0, 2.0);
    gis::BoundingBox bbox2(1.0, 1.0, 3.0, 3.0);
    
    bool intersects = bbox1.intersects(bbox2);
    bool contains_point = bbox1.contains(gis::Point2D(1.0, 1.0));
    double area = bbox1.area();
    
    std::cout << "   Bounding boxes intersect: " << (intersects ? "Yes" : "No") << "\n";
    std::cout << "   BBox1 contains (1,1): " << (contains_point ? "Yes" : "No") << "\n";
    std::cout << "   BBox1 area: " << area << "\n";
    
    std::cout << "\n";
}

void demonstratePerformance() {
    std::cout << "=== Performance Demo ===\n\n";
    
    std::cout << "1. Geometry creation performance...\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create many point geometries
    const int num_points = 10000;
    std::vector<std::unique_ptr<gis::PointGeometry>> points;
    points.reserve(num_points);
    
    for (int i = 0; i < num_points; ++i) {
        double x = static_cast<double>(i % 100);
        double y = static_cast<double>(i / 100);
        points.push_back(std::make_unique<gis::PointGeometry>(gis::Point2D(x, y)));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "   Created " << num_points << " points in " << duration.count() << " μs\n";
    std::cout << "   Rate: " << (num_points * 1000000.0 / duration.count()) << " points/second\n";
    
    std::cout << "\n2. Bounding box calculations...\n";
    
    start = std::chrono::high_resolution_clock::now();
    
    // Calculate bounding boxes for all points
    std::vector<gis::BoundingBox> bounds;
    bounds.reserve(num_points);
    
    for (const auto& point : points) {
        bounds.push_back(point->getBounds());
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "   Calculated " << bounds.size() << " bounding boxes in " << duration.count() << " μs\n";
    std::cout << "   Rate: " << (bounds.size() * 1000000.0 / duration.count()) << " calculations/second\n";
    
    std::cout << "\n";
}

int main() {
    std::cout << "GIS Shapefile Processor - Basic Usage Examples\n";
    std::cout << std::string(50, '=') << "\n\n";
    
    try {
        demonstrateShapefileReading();
        demonstrateGeocoding();
        demonstrateSpatialOperations();
        demonstratePerformance();
        
        std::cout << "=== Demo Complete ===\n\n";
        
        std::cout << "Next Steps:\n";
        std::cout << "1. Obtain real shapefile data (.shp, .shx, .dbf files)\n";
        std::cout << "2. Use shp-info tool to inspect your data\n";
        std::cout << "3. Load address data into geocoder for real geocoding\n";
        std::cout << "4. Build spatial indices for large datasets\n";
        std::cout << "5. Integrate with your applications\n\n";
        
        std::cout << "For more examples, see:\n";
        std::cout << "- examples/performance_demo.cpp (advanced performance testing)\n";
        std::cout << "- tools/geocoder_cli.cpp (command-line geocoding)\n";
        std::cout << "- tools/spatial_query.cpp (spatial analysis)\n\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

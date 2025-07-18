#include "gis/shapefile_reader.h"
#include <iostream>
#include <iomanip>
#include <chrono>

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " <shapefile_path>\n";
    std::cout << "  shapefile_path: Path to shapefile (without .shp extension)\n";
    std::cout << "\nExample:\n";
    std::cout << "  " << program_name << " data/cities\n";
    std::cout << "  This will read data/cities.shp, data/cities.shx, and data/cities.dbf\n";
}

void printGeometryInfo(const gis::Geometry* geometry) {
    if (!geometry) {
        std::cout << "    Geometry: NULL\n";
        return;
    }
    
    gis::BoundingBox bounds = geometry->getBounds();
    
    switch (geometry->getType()) {
        case gis::ShapeType::Point:
            std::cout << "    Geometry: Point\n";
            break;
        case gis::ShapeType::PolyLine:
            std::cout << "    Geometry: Polyline\n";
            break;
        case gis::ShapeType::Polygon:
            std::cout << "    Geometry: Polygon\n";
            break;
        default:
            std::cout << "    Geometry: Type " << static_cast<int>(geometry->getType()) << "\n";
            break;
    }
    
    std::cout << "    Bounds: (" << std::fixed << std::setprecision(6) 
              << bounds.min_x << ", " << bounds.min_y << ") to ("
              << bounds.max_x << ", " << bounds.max_y << ")\n";
}

void printAttributes(const std::unordered_map<std::string, gis::FieldValue>& attributes) {
    if (attributes.empty()) {
        std::cout << "    Attributes: None\n";
        return;
    }
    
    std::cout << "    Attributes:\n";
    for (const auto& attr : attributes) {
        std::cout << "      " << attr.first << ": ";
        
        std::visit([](const auto& value) {
            std::cout << value;
        }, attr.second);
        
        std::cout << "\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string shapefile_path = argv[1];
    
    std::cout << "=== Shapefile Information Tool ===\n\n";
    
    // Create and open shapefile reader
    gis::ShapefileReader reader(shapefile_path);
    
    if (!reader.open()) {
        std::cerr << "Error: Failed to open shapefile: " << shapefile_path << std::endl;
        return 1;
    }
    
    // Print general information
    std::cout << reader.getInfo() << "\n";
    
    // Print first few records for sample
    uint32_t record_count = reader.getRecordCount();
    uint32_t sample_count = std::min(5u, record_count);
    
    if (sample_count > 0) {
        std::cout << "Sample Records (showing first " << sample_count << "):\n";
        std::cout << std::string(50, '-') << "\n";
        
        for (uint32_t i = 0; i < sample_count; ++i) {
            auto record = reader.readRecord(i);
            if (record) {
                std::cout << "Record #" << record->record_number << ":\n";
                printGeometryInfo(record->geometry.get());
                printAttributes(record->attributes);
                std::cout << "\n";
            }
        }
    }
    
    // Performance test for larger files
    if (record_count > 100) {
        std::cout << "Performance Test:\n";
        std::cout << std::string(20, '-') << "\n";
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Read all records
        auto all_records = reader.readAllRecords();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Read " << all_records.size() << " records in " 
                  << duration.count() << " ms\n";
        std::cout << "Rate: " << (all_records.size() * 1000.0 / duration.count()) 
                  << " records/second\n";
    }
    
    std::cout << "\n=== Analysis Complete ===\n";
    
    return 0;
}

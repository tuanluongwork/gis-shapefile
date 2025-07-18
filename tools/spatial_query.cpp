#include "gis/shapefile_reader.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <sstream>
#include <chrono>

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " <shapefile_path> [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -v, --verbose     Show detailed information\n";
    std::cout << "  -q, --query       Interactive spatial query mode\n";
    std::cout << "  -b, --bounds      Show bounding box details\n";
    std::cout << "  -r, --records     Show record samples\n";
    std::cout << "  -h, --help        Show this help\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program_name << " data/cities -v\n";
    std::cout << "  " << program_name << " data/boundaries --query\n";
}

void runSpatialQuery(gis::ShapefileReader& reader) {
    std::cout << "\n=== Interactive Spatial Query Mode ===\n";
    std::cout << "Commands:\n";
    std::cout << "  bbox <minx> <miny> <maxx> <maxy>  - Query by bounding box\n";
    std::cout << "  point <x> <y>                     - Find containing geometries\n";
    std::cout << "  info                              - Show shapefile info\n";
    std::cout << "  quit                              - Exit query mode\n\n";
    
    std::string line;
    while (true) {
        std::cout << "spatial> ";
        std::getline(std::cin, line);
        
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "quit" || command == "exit") {
            break;
        } else if (command == "info") {
            std::cout << reader.getInfo() << "\n";
        } else if (command == "bbox") {
            double minx, miny, maxx, maxy;
            if (!(iss >> minx >> miny >> maxx >> maxy)) {
                std::cout << "Usage: bbox <minx> <miny> <maxx> <maxy>\n";
                continue;
            }
            
            gis::BoundingBox query_bounds(minx, miny, maxx, maxy);
            std::cout << "Querying bounding box: (" << minx << ", " << miny 
                      << ") to (" << maxx << ", " << maxy << ")\n";
            
            auto records = reader.readRecordsInBounds(query_bounds);
            std::cout << "Found " << records.size() << " intersecting records\n";
            
            for (size_t i = 0; i < std::min(5ul, records.size()); ++i) {
                const auto& record = records[i];
                if (record && record->geometry) {
                    gis::BoundingBox bounds = record->geometry->getBounds();
                    std::cout << "  Record " << record->record_number 
                              << ": bounds (" << bounds.min_x << ", " << bounds.min_y 
                              << ") to (" << bounds.max_x << ", " << bounds.max_y << ")\n";
                }
            }
            if (records.size() > 5) {
                std::cout << "  ... and " << (records.size() - 5) << " more\n";
            }
            
        } else if (command == "point") {
            double x, y;
            if (!(iss >> x >> y)) {
                std::cout << "Usage: point <x> <y>\n";
                continue;
            }
            
            gis::Point2D query_point(x, y);
            std::cout << "Searching for geometries containing point (" << x << ", " << y << ")\n";
            
            // Create small bounding box around point for initial filtering
            double epsilon = 0.001;
            gis::BoundingBox point_bounds(x - epsilon, y - epsilon, x + epsilon, y + epsilon);
            auto candidates = reader.readRecordsInBounds(point_bounds);
            
            int found_count = 0;
            for (const auto& record : candidates) {
                if (record && record->geometry) {
                    if (record->geometry->getType() == gis::ShapeType::Polygon) {
                        auto* polygon = dynamic_cast<gis::PolygonGeometry*>(record->geometry.get());
                        if (polygon && polygon->contains(query_point)) {
                            std::cout << "  Found in Record " << record->record_number << "\n";
                            found_count++;
                        }
                    }
                }
            }
            
            if (found_count == 0) {
                std::cout << "  Point not found in any polygon geometries\n";
            }
            
        } else {
            std::cout << "Unknown command: " << command << "\n";
            std::cout << "Type 'quit' to exit.\n";
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string shapefile_path = argv[1];
    bool verbose = false;
    bool query_mode = false;
    bool show_bounds = false;
    bool show_records = false;
    
    // Parse command line options
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "-q" || arg == "--query") {
            query_mode = true;
        } else if (arg == "-b" || arg == "--bounds") {
            show_bounds = true;
        } else if (arg == "-r" || arg == "--records") {
            show_records = true;
        } else if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
    }
    
    std::cout << "=== Spatial Query Tool ===\n\n";
    
    gis::ShapefileReader reader(shapefile_path);
    
    if (!reader.open()) {
        std::cerr << "Error: Failed to open shapefile: " << shapefile_path << std::endl;
        return 1;
    }
    
    std::cout << "Opened shapefile: " << shapefile_path << "\n";
    std::cout << reader.getInfo() << "\n";
    
    if (show_bounds) {
        gis::BoundingBox bounds = reader.getBounds();
        std::cout << "Detailed Bounds Information:\n";
        std::cout << "  Min X: " << std::fixed << std::setprecision(8) << bounds.min_x << "\n";
        std::cout << "  Min Y: " << std::fixed << std::setprecision(8) << bounds.min_y << "\n";
        std::cout << "  Max X: " << std::fixed << std::setprecision(8) << bounds.max_x << "\n";
        std::cout << "  Max Y: " << std::fixed << std::setprecision(8) << bounds.max_y << "\n";
        std::cout << "  Width: " << (bounds.max_x - bounds.min_x) << "\n";
        std::cout << "  Height: " << (bounds.max_y - bounds.min_y) << "\n";
        std::cout << "  Area: " << bounds.area() << "\n\n";
    }
    
    if (show_records) {
        uint32_t sample_count = std::min(10u, reader.getRecordCount());
        std::cout << "Sample Records (first " << sample_count << "):\n";
        std::cout << std::string(60, '-') << "\n";
        
        for (uint32_t i = 0; i < sample_count; ++i) {
            auto record = reader.readRecord(i);
            if (record) {
                std::cout << "Record #" << record->record_number << ":\n";
                
                if (record->geometry) {
                    gis::BoundingBox geom_bounds = record->geometry->getBounds();
                    std::cout << "  Geometry Type: ";
                    switch (record->geometry->getType()) {
                        case gis::ShapeType::Point:
                            std::cout << "Point\n";
                            break;
                        case gis::ShapeType::PolyLine:
                            std::cout << "Polyline\n";
                            break;
                        case gis::ShapeType::Polygon:
                            std::cout << "Polygon\n";
                            break;
                        default:
                            std::cout << "Type " << static_cast<int>(record->geometry->getType()) << "\n";
                            break;
                    }
                    std::cout << "  Bounds: (" << geom_bounds.min_x << ", " << geom_bounds.min_y 
                              << ") to (" << geom_bounds.max_x << ", " << geom_bounds.max_y << ")\n";
                }
                
                if (verbose && !record->attributes.empty()) {
                    std::cout << "  Attributes:\n";
                    for (const auto& attr : record->attributes) {
                        std::cout << "    " << attr.first << ": ";
                        std::visit([](const auto& value) {
                            std::cout << value;
                        }, attr.second);
                        std::cout << "\n";
                    }
                }
                std::cout << "\n";
            }
        }
    }
    
    if (query_mode) {
        runSpatialQuery(reader);
    }
    
    return 0;
}

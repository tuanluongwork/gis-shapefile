#include "gis/geocoder.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <sstream>

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] <command> [arguments]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  load <shapefile>          Load address data from shapefile\n";
    std::cout << "  geocode <address>         Geocode a single address\n";
    std::cout << "  reverse <x> <y>           Reverse geocode coordinates\n";
    std::cout << "  batch <file>              Batch geocode addresses from file\n";
    std::cout << "  interactive               Start interactive mode\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program_name << " load data/addresses\n";
    std::cout << "  " << program_name << " geocode \"123 Main St, Anytown, CA 12345\"\n";
    std::cout << "  " << program_name << " reverse -122.4194 37.7749\n";
    std::cout << "  " << program_name << " interactive\n";
}

void printGeocodeResult(const gis::GeocodeResult& result) {
    if (result.confidence_score == 0.0) {
        std::cout << "No match found.\n";
        return;
    }
    
    std::cout << "Match Found:\n";
    std::cout << "  Coordinates: " << std::fixed << std::setprecision(6)
              << result.coordinate.x << ", " << result.coordinate.y << "\n";
    std::cout << "  Address: " << result.matched_address.toString() << "\n";
    std::cout << "  Confidence: " << std::fixed << std::setprecision(2) 
              << (result.confidence_score * 100.0) << "%\n";
    std::cout << "  Match Type: " << result.match_type << "\n";
}

void runInteractiveMode(gis::Geocoder& geocoder) {
    std::cout << "\n=== Interactive Geocoding Mode ===\n";
    std::cout << "Commands:\n";
    std::cout << "  geocode <address>     - Geocode an address\n";
    std::cout << "  reverse <x> <y>       - Reverse geocode coordinates\n";
    std::cout << "  stats                 - Show geocoder statistics\n";
    std::cout << "  help                  - Show this help\n";
    std::cout << "  quit                  - Exit interactive mode\n\n";
    
    std::string line;
    while (true) {
        std::cout << "geocoder> ";
        std::getline(std::cin, line);
        
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "quit" || command == "exit") {
            break;
        } else if (command == "help") {
            std::cout << "Commands: geocode, reverse, stats, help, quit\n";
        } else if (command == "stats") {
            std::cout << geocoder.getStats() << "\n";
        } else if (command == "geocode") {
            std::string address;
            std::getline(iss, address);
            if (!address.empty() && address[0] == ' ') {
                address = address.substr(1);  // Remove leading space
            }
            
            if (address.empty()) {
                std::cout << "Usage: geocode <address>\n";
                continue;
            }
            
            auto start = std::chrono::high_resolution_clock::now();
            gis::GeocodeResult result = geocoder.geocode(address);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            printGeocodeResult(result);
            std::cout << "  Query Time: " << duration.count() << " μs\n\n";
            
        } else if (command == "reverse") {
            double x, y;
            if (!(iss >> x >> y)) {
                std::cout << "Usage: reverse <x> <y>\n";
                continue;
            }
            
            auto start = std::chrono::high_resolution_clock::now();
            gis::GeocodeResult result = geocoder.reverseGeocode(gis::Point2D(x, y));
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            printGeocodeResult(result);
            std::cout << "  Query Time: " << duration.count() << " μs\n\n";
            
        } else {
            std::cout << "Unknown command: " << command << "\n";
            std::cout << "Type 'help' for available commands.\n";
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string command = argv[1];
    
    if (command == "load") {
        if (argc != 3) {
            std::cout << "Usage: " << argv[0] << " load <shapefile>\n";
            return 1;
        }
        
        std::string shapefile_path = argv[2];
        
        std::cout << "Loading address data from: " << shapefile_path << "\n";
        
        gis::Geocoder geocoder;
        if (!geocoder.loadAddressData(shapefile_path)) {
            std::cerr << "Error: Failed to load address data from " << shapefile_path << std::endl;
            return 1;
        }
        
        std::cout << "Address data loaded successfully!\n";
        std::cout << geocoder.getStats() << "\n";
        
    } else if (command == "geocode") {
        if (argc != 3) {
            std::cout << "Usage: " << argv[0] << " geocode \"<address>\"\n";
            return 1;
        }
        
        std::string address = argv[2];
        
        // For demo purposes, create a simple geocoder
        // In real usage, you'd load data first
        gis::Geocoder geocoder;
        
        std::cout << "Geocoding: " << address << "\n";
        std::cout << std::string(40, '-') << "\n";
        
        gis::GeocodeResult result = geocoder.geocode(address);
        printGeocodeResult(result);
        
    } else if (command == "reverse") {
        if (argc != 4) {
            std::cout << "Usage: " << argv[0] << " reverse <x> <y>\n";
            return 1;
        }
        
        double x = std::stod(argv[2]);
        double y = std::stod(argv[3]);
        
        gis::Geocoder geocoder;
        
        std::cout << "Reverse geocoding: " << x << ", " << y << "\n";
        std::cout << std::string(40, '-') << "\n";
        
        gis::GeocodeResult result = geocoder.reverseGeocode(gis::Point2D(x, y));
        printGeocodeResult(result);
        
    } else if (command == "interactive") {
        gis::Geocoder geocoder;
        
        // Try to load default data if available
        if (argc > 2) {
            std::string shapefile_path = argv[2];
            std::cout << "Loading address data from: " << shapefile_path << "\n";
            if (geocoder.loadAddressData(shapefile_path)) {
                std::cout << "Address data loaded successfully!\n";
            } else {
                std::cout << "Warning: Failed to load address data. Continuing with empty geocoder.\n";
            }
        }
        
        runInteractiveMode(geocoder);
        
    } else if (command == "batch") {
        if (argc != 3) {
            std::cout << "Usage: " << argv[0] << " batch <addresses_file>\n";
            return 1;
        }
        
        std::string filename = argv[2];
        std::ifstream file(filename);
        
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file " << filename << std::endl;
            return 1;
        }
        
        gis::Geocoder geocoder;
        std::vector<std::string> addresses;
        std::string line;
        
        while (std::getline(file, line)) {
            if (!line.empty()) {
                addresses.push_back(line);
            }
        }
        
        std::cout << "Batch geocoding " << addresses.size() << " addresses...\n";
        
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<gis::GeocodeResult> results = geocoder.geocodeBatch(addresses);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // Print results
        for (size_t i = 0; i < results.size(); ++i) {
            std::cout << "\nAddress " << (i + 1) << ": " << addresses[i] << "\n";
            printGeocodeResult(results[i]);
        }
        
        std::cout << "\nBatch Processing Summary:\n";
        std::cout << "  Total addresses: " << addresses.size() << "\n";
        std::cout << "  Processing time: " << duration.count() << " ms\n";
        std::cout << "  Rate: " << (addresses.size() * 1000.0 / duration.count()) << " addresses/second\n";
        
    } else {
        std::cout << "Unknown command: " << command << "\n";
        printUsage(argv[0]);
        return 1;
    }
    
    return 0;
}

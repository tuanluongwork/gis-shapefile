#include "http_server.h"
#include "gis/geocoder.h"
#include "gis/shapefile_reader.h"
#include <iostream>
#include <sstream>
#include <regex>
#include <iomanip>
#include <chrono>

class GeocodingAPI {
private:
    gis::Geocoder geocoder_;
    bool data_loaded_;
    
public:
    GeocodingAPI() : data_loaded_(false) {}
    
    bool loadData(const std::string& shapefile_path) {
        if (geocoder_.loadAddressData(shapefile_path)) {
            data_loaded_ = true;
            std::cout << "Loaded geocoding data from: " << shapefile_path << std::endl;
            return true;
        }
        return false;
    }
    
    std::string handleRequest(const std::string& path, const std::string& query) {
        if (path == "/") {
            return createWelcomeResponse();
        } else if (path == "/geocode") {
            return handleGeocode(query);
        } else if (path == "/reverse") {
            return handleReverseGeocode(query);
        } else if (path == "/health") {
            return createHealthResponse();
        } else if (path == "/stats") {
            return createStatsResponse();
        } else {
            return createErrorResponse("Not Found", 404);
        }
    }
    
private:
    std::string createWelcomeResponse() {
        std::ostringstream json;
        json << "{\n";
        json << "  \"service\": \"GIS Shapefile Geocoding API\",\n";
        json << "  \"version\": \"1.0.0\",\n";
        json << "  \"author\": \"Tuan Luong\",\n";
        json << "  \"endpoints\": {\n";
        json << "    \"GET /geocode?address=<address>\": \"Geocode an address\",\n";
        json << "    \"GET /reverse?lat=<lat>&lng=<lng>\": \"Reverse geocode coordinates\",\n";
        json << "    \"GET /health\": \"Health check\",\n";
        json << "    \"GET /stats\": \"Service statistics\"\n";
        json << "  },\n";
        json << "  \"data_loaded\": " << (data_loaded_ ? "true" : "false") << ",\n";
        json << "  \"description\": \"Modern C++ GIS library for enterprise geocoding systems\"\n";
        json << "}";
        return json.str();
    }
    
    std::string handleGeocode(const std::string& query) {
        if (!data_loaded_) {
            return createErrorResponse("No geocoding data loaded");
        }
        
        std::string address = extractParameter(query, "address");
        if (address.empty()) {
            return createErrorResponse("Missing 'address' parameter");
        }
        
        // URL decode address
        address = urlDecode(address);
        
        gis::GeocodeResult result = geocoder_.geocode(address);
        
        std::ostringstream json;
        json << "{\n";
        json << "  \"input_address\": \"" << escapeJson(address) << "\",\n";
        json << "  \"success\": " << (result.confidence_score > 0 ? "true" : "false") << ",\n";
        
        if (result.confidence_score > 0) {
            json << "  \"result\": {\n";
            json << "    \"latitude\": " << std::fixed << std::setprecision(8) << result.coordinate.y << ",\n";
            json << "    \"longitude\": " << std::fixed << std::setprecision(8) << result.coordinate.x << ",\n";
            json << "    \"matched_address\": \"" << escapeJson(result.matched_address.toString()) << "\",\n";
            json << "    \"confidence\": " << std::setprecision(3) << result.confidence_score << ",\n";
            json << "    \"match_type\": \"" << result.match_type << "\"\n";
            json << "  }\n";
        } else {
            json << "  \"error\": \"No match found\"\n";
        }
        
        json << "}";
        return json.str();
    }
    
    std::string handleReverseGeocode(const std::string& query) {
        if (!data_loaded_) {
            return createErrorResponse("No geocoding data loaded");
        }
        
        std::string lat_str = extractParameter(query, "lat");
        std::string lng_str = extractParameter(query, "lng");
        
        if (lat_str.empty() || lng_str.empty()) {
            return createErrorResponse("Missing 'lat' or 'lng' parameter");
        }
        
        try {
            double lat = std::stod(lat_str);
            double lng = std::stod(lng_str);
            
            gis::Point2D point(lng, lat);  // Note: GIS convention is (x=lng, y=lat)
            gis::GeocodeResult result = geocoder_.reverseGeocode(point);
            
            std::ostringstream json;
            json << "{\n";
            json << "  \"input_coordinates\": {\n";
            json << "    \"latitude\": " << std::fixed << std::setprecision(8) << lat << ",\n";
            json << "    \"longitude\": " << std::fixed << std::setprecision(8) << lng << "\n";
            json << "  },\n";
            json << "  \"success\": " << (result.confidence_score > 0 ? "true" : "false") << ",\n";
            
            if (result.confidence_score > 0) {
                json << "  \"result\": {\n";
                json << "    \"address\": \"" << escapeJson(result.matched_address.toString()) << "\",\n";
                json << "    \"confidence\": " << std::setprecision(3) << result.confidence_score << ",\n";
                json << "    \"match_type\": \"" << result.match_type << "\"\n";
                json << "  }\n";
            } else {
                json << "  \"error\": \"No address found at coordinates\"\n";
            }
            
            json << "}";
            return json.str();
            
        } catch (const std::exception& e) {
            return createErrorResponse("Invalid coordinates");
        }
    }
    
    std::string createHealthResponse() {
        std::ostringstream json;
        json << "{\n";
        json << "  \"status\": \"healthy\",\n";
        json << "  \"data_loaded\": " << (data_loaded_ ? "true" : "false") << ",\n";
        json << "  \"timestamp\": \"" << getCurrentTimestamp() << "\"\n";
        json << "}";
        return json.str();
    }
    
    std::string createStatsResponse() {
        std::ostringstream json;
        json << "{\n";
        json << "  \"service\": \"GIS Geocoding API\",\n";
        json << "  \"data_loaded\": " << (data_loaded_ ? "true" : "false") << ",\n";
        
        if (data_loaded_) {
            json << "  \"geocoder_stats\": " << escapeJson(geocoder_.getStats()) << ",\n";
        }
        
        json << "  \"timestamp\": \"" << getCurrentTimestamp() << "\"\n";
        json << "}";
        return json.str();
    }
    
    std::string createErrorResponse(const std::string& message, int code = 400) {
        std::ostringstream json;
        json << "{\n";
        json << "  \"error\": \"" << escapeJson(message) << "\",\n";
        json << "  \"code\": " << code << "\n";
        json << "}";
        return json.str();
    }
    
    std::string extractParameter(const std::string& query, const std::string& param) {
        std::regex param_regex(param + "=([^&]+)");
        std::smatch match;
        if (std::regex_search(query, match, param_regex)) {
            return match[1].str();
        }
        return "";
    }
    
    std::string urlDecode(const std::string& str) {
        std::string result;
        for (size_t i = 0; i < str.length(); ++i) {
            if (str[i] == '%' && i + 2 < str.length()) {
                int hex_value;
                std::istringstream iss(str.substr(i + 1, 2));
                if (iss >> std::hex >> hex_value) {
                    result += static_cast<char>(hex_value);
                    i += 2;
                } else {
                    result += str[i];
                }
            } else if (str[i] == '+') {
                result += ' ';
            } else {
                result += str[i];
            }
        }
        return result;
    }
    
    std::string escapeJson(const std::string& str) {
        std::string result;
        for (char c : str) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c; break;
            }
        }
        return result;
    }
    
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::ostringstream oss;
        oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
        return oss.str();
    }
};

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -p, --port <port>     Server port (default: 8080)\n";
    std::cout << "  -d, --data <path>     Path to shapefile data\n";
    std::cout << "  -h, --help            Show this help\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program_name << " --port 8080 --data data/addresses\n";
    std::cout << "  " << program_name << " -p 9000 -d /path/to/geocoding/data\n\n";
    std::cout << "API Endpoints:\n";
    std::cout << "  GET /                                 - API information\n";
    std::cout << "  GET /geocode?address=<address>        - Geocode address\n";
    std::cout << "  GET /reverse?lat=<lat>&lng=<lng>      - Reverse geocode\n";
    std::cout << "  GET /health                           - Health check\n";
    std::cout << "  GET /stats                            - Service statistics\n";
}

int main(int argc, char* argv[]) {
    int port = 8080;
    std::string data_path;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if ((arg == "-p" || arg == "--port") && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if ((arg == "-d" || arg == "--data") && i + 1 < argc) {
            data_path = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
    }
    
    std::cout << "=== GIS Geocoding API Server ===\n\n";
    
    GeocodingAPI api;
    
    // Load data if provided
    if (!data_path.empty()) {
        if (!api.loadData(data_path)) {
            std::cerr << "Warning: Failed to load data from " << data_path << std::endl;
            std::cerr << "Server will start but geocoding will not work.\n" << std::endl;
        }
    } else {
        std::cout << "No data path provided. Use --data option to load geocoding data.\n" << std::endl;
    }
    
    // Create and start server
    gis::HttpServer server(port);
    server.setHandler([&api](const std::string& path, const std::string& query) {
        return api.handleRequest(path, query);
    });
    
    if (!server.start()) {
        std::cerr << "Failed to start server on port " << port << std::endl;
        return 1;
    }
    
    std::cout << "Server running on http://localhost:" << port << std::endl;
    std::cout << "Press Enter to stop the server..." << std::endl;
    
    std::cin.get();
    
    std::cout << "Stopping server..." << std::endl;
    server.stop();
    
    return 0;
}

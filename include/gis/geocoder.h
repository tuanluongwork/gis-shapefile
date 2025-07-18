#pragma once

#include "geometry.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace gis {

/**
 * @brief Represents a parsed address with components
 */
struct ParsedAddress {
    std::string house_number;
    std::string street_name;
    std::string street_type;
    std::string city;
    std::string state;
    std::string zip_code;
    std::string country;
    
    std::string full_address;
    
    /**
     * @brief Check if the address has minimum required components
     */
    bool isValid() const;
    
    /**
     * @brief Convert to string representation
     */
    std::string toString() const;
};

/**
 * @brief Result of geocoding operation with confidence score
 */
struct GeocodeResult {
    Point2D coordinate;
    ParsedAddress matched_address;
    double confidence_score;  // 0.0 to 1.0
    std::string match_type;   // "exact", "interpolated", "centroid"
    
    GeocodeResult() : confidence_score(0.0) {}
    GeocodeResult(const Point2D& coord, const ParsedAddress& addr, double confidence)
        : coordinate(coord), matched_address(addr), confidence_score(confidence) {}
};

/**
 * @brief Address parser for extracting components from address strings
 */
class AddressParser {
private:
    std::unordered_map<std::string, std::string> street_type_abbreviations_;
    std::unordered_map<std::string, std::string> state_abbreviations_;
    
public:
    AddressParser();
    
    /**
     * @brief Parse an address string into components
     * @param address_string Raw address string
     * @return Parsed address structure
     */
    ParsedAddress parse(const std::string& address_string) const;
    
    /**
     * @brief Normalize an address string for matching
     * @param address Raw address string
     * @return Normalized address string
     */
    std::string normalize(const std::string& address) const;

private:
    void initializeAbbreviations();
    std::vector<std::string> tokenize(const std::string& text) const;
    std::string expandAbbreviations(const std::string& text) const;
    bool isNumeric(const std::string& str) const;
    bool isZipCode(const std::string& str) const;
};

/**
 * @brief Main geocoding engine
 * 
 * Provides address-to-coordinate conversion using shapefile data
 * and various matching algorithms.
 */
class Geocoder {
private:
    struct AddressIndex {
        std::unordered_map<std::string, std::vector<size_t>> street_index;
        std::unordered_map<std::string, std::vector<size_t>> city_index;
        std::unordered_map<std::string, std::vector<size_t>> zip_index;
    };
    
    std::vector<std::unique_ptr<ShapeRecord>> address_data_;
    AddressIndex index_;
    AddressParser parser_;
    
public:
    Geocoder();
    ~Geocoder();
    
    /**
     * @brief Load address data from shapefile
     * @param shapefile_path Path to address shapefile
     * @param address_field Name of the field containing addresses
     * @return true if successful
     */
    bool loadAddressData(const std::string& shapefile_path, 
                        const std::string& address_field = "ADDRESS");
    
    /**
     * @brief Geocode a single address
     * @param address Address string to geocode
     * @return Geocoding result with coordinate and confidence
     */
    GeocodeResult geocode(const std::string& address) const;
    
    /**
     * @brief Geocode multiple addresses in batch
     * @param addresses Vector of address strings
     * @return Vector of geocoding results
     */
    std::vector<GeocodeResult> geocodeBatch(const std::vector<std::string>& addresses) const;
    
    /**
     * @brief Reverse geocode - find address from coordinates
     * @param point Coordinate point
     * @param max_distance Maximum search distance in meters
     * @return Closest address or empty result
     */
    GeocodeResult reverseGeocode(const Point2D& point, double max_distance = 100.0) const;
    
    /**
     * @brief Get statistics about loaded data
     * @return String with data statistics
     */
    std::string getStats() const;

private:
    void buildIndex();
    std::vector<GeocodeResult> findCandidates(const ParsedAddress& parsed_address) const;
    double calculateConfidence(const ParsedAddress& input, const ParsedAddress& candidate) const;
    double calculateDistance(const Point2D& p1, const Point2D& p2) const;
    std::string extractAddressFromRecord(const ShapeRecord& record, const std::string& field_name) const;
    
    // Fuzzy matching algorithms
    double levenshteinDistance(const std::string& s1, const std::string& s2) const;
    double jaroWinklerSimilarity(const std::string& s1, const std::string& s2) const;
};

} // namespace gis

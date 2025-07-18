#include "gis/geocoder.h"
#include "gis/shapefile_reader.h"
#include <algorithm>
#include <sstream>
#include <regex>
#include <cmath>
#include <cctype>

namespace gis {

// AddressParser implementation
AddressParser::AddressParser() {
    initializeAbbreviations();
}

void AddressParser::initializeAbbreviations() {
    // Common street type abbreviations
    street_type_abbreviations_ = {
        {"ST", "STREET"}, {"AVE", "AVENUE"}, {"BLVD", "BOULEVARD"},
        {"RD", "ROAD"}, {"DR", "DRIVE"}, {"LN", "LANE"},
        {"CT", "COURT"}, {"PL", "PLACE"}, {"WAY", "WAY"},
        {"CIR", "CIRCLE"}, {"PKWY", "PARKWAY"}, {"HWY", "HIGHWAY"}
    };
    
    // State abbreviations
    state_abbreviations_ = {
        {"AL", "ALABAMA"}, {"AK", "ALASKA"}, {"AZ", "ARIZONA"},
        {"AR", "ARKANSAS"}, {"CA", "CALIFORNIA"}, {"CO", "COLORADO"},
        {"CT", "CONNECTICUT"}, {"DE", "DELAWARE"}, {"FL", "FLORIDA"},
        {"GA", "GEORGIA"}, {"HI", "HAWAII"}, {"ID", "IDAHO"},
        {"IL", "ILLINOIS"}, {"IN", "INDIANA"}, {"IA", "IOWA"},
        {"KS", "KANSAS"}, {"KY", "KENTUCKY"}, {"LA", "LOUISIANA"},
        {"ME", "MAINE"}, {"MD", "MARYLAND"}, {"MA", "MASSACHUSETTS"},
        {"MI", "MICHIGAN"}, {"MN", "MINNESOTA"}, {"MS", "MISSISSIPPI"},
        {"MO", "MISSOURI"}, {"MT", "MONTANA"}, {"NE", "NEBRASKA"},
        {"NV", "NEVADA"}, {"NH", "NEW HAMPSHIRE"}, {"NJ", "NEW JERSEY"},
        {"NM", "NEW MEXICO"}, {"NY", "NEW YORK"}, {"NC", "NORTH CAROLINA"},
        {"ND", "NORTH DAKOTA"}, {"OH", "OHIO"}, {"OK", "OKLAHOMA"},
        {"OR", "OREGON"}, {"PA", "PENNSYLVANIA"}, {"RI", "RHODE ISLAND"},
        {"SC", "SOUTH CAROLINA"}, {"SD", "SOUTH DAKOTA"}, {"TN", "TENNESSEE"},
        {"TX", "TEXAS"}, {"UT", "UTAH"}, {"VT", "VERMONT"},
        {"VA", "VIRGINIA"}, {"WA", "WASHINGTON"}, {"WV", "WEST VIRGINIA"},
        {"WI", "WISCONSIN"}, {"WY", "WYOMING"}, {"DC", "DISTRICT OF COLUMBIA"}
    };
}

ParsedAddress AddressParser::parse(const std::string& address_string) const {
    ParsedAddress address;
    address.full_address = address_string;
    
    std::vector<std::string> tokens = tokenize(normalize(address_string));
    
    if (tokens.empty()) return address;
    
    size_t i = 0;
    
    // Extract house number (first numeric token)
    if (i < tokens.size() && isNumeric(tokens[i])) {
        address.house_number = tokens[i];
        i++;
    }
    
    // Extract street name and type
    std::vector<std::string> street_parts;
    while (i < tokens.size() && !isZipCode(tokens[i]) && 
           state_abbreviations_.find(tokens[i]) == state_abbreviations_.end()) {
        street_parts.push_back(tokens[i]);
        i++;
    }
    
    if (!street_parts.empty()) {
        // Last token might be street type
        std::string last_token = street_parts.back();
        if (street_type_abbreviations_.find(last_token) != street_type_abbreviations_.end()) {
            address.street_type = street_type_abbreviations_.at(last_token);
            street_parts.pop_back();
        }
        
        // Join remaining tokens as street name
        std::ostringstream oss;
        for (size_t j = 0; j < street_parts.size(); ++j) {
            if (j > 0) oss << " ";
            oss << street_parts[j];
        }
        address.street_name = oss.str();
    }
    
    // Extract state
    if (i < tokens.size() && state_abbreviations_.find(tokens[i]) != state_abbreviations_.end()) {
        address.state = tokens[i];
        i++;
    }
    
    // Extract zip code
    if (i < tokens.size() && isZipCode(tokens[i])) {
        address.zip_code = tokens[i];
        i++;
    }
    
    // Remaining tokens are likely city
    if (i < tokens.size()) {
        std::ostringstream oss;
        for (size_t j = 0; j < i; ++j) {
            if (j > 0) oss << " ";
            oss << tokens[j];
        }
        address.city = oss.str();
    }
    
    return address;
}

std::string AddressParser::normalize(const std::string& address) const {
    std::string normalized = address;
    
    // Convert to uppercase
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), 
                   [](unsigned char c) { return std::toupper(c); });
    
    // Remove common punctuation
    std::regex punct_regex("[,\\.]");
    normalized = std::regex_replace(normalized, punct_regex, " ");
    
    // Collapse multiple spaces
    std::regex spaces_regex("\\s+");
    normalized = std::regex_replace(normalized, spaces_regex, " ");
    
    // Trim
    normalized.erase(0, normalized.find_first_not_of(" \t"));
    normalized.erase(normalized.find_last_not_of(" \t") + 1);
    
    return expandAbbreviations(normalized);
}

std::vector<std::string> AddressParser::tokenize(const std::string& text) const {
    std::vector<std::string> tokens;
    std::istringstream iss(text);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string AddressParser::expandAbbreviations(const std::string& text) const {
    std::string expanded = text;
    
    // This is a simplified expansion - in production, you'd want more sophisticated handling
    for (const auto& abbrev : street_type_abbreviations_) {
        std::regex abbrev_regex("\\b" + abbrev.first + "\\b");
        expanded = std::regex_replace(expanded, abbrev_regex, abbrev.second);
    }
    
    return expanded;
}

bool AddressParser::isNumeric(const std::string& str) const {
    return !str.empty() && std::all_of(str.begin(), str.end(), ::isdigit);
}

bool AddressParser::isZipCode(const std::string& str) const {
    // Simple check for 5 or 9 digit zip codes
    if (str.length() == 5) {
        return std::all_of(str.begin(), str.end(), ::isdigit);
    }
    if (str.length() == 10 && str[5] == '-') {
        return std::all_of(str.begin(), str.begin() + 5, ::isdigit) &&
               std::all_of(str.begin() + 6, str.end(), ::isdigit);
    }
    return false;
}

bool ParsedAddress::isValid() const {
    return !house_number.empty() && !street_name.empty();
}

std::string ParsedAddress::toString() const {
    std::ostringstream oss;
    if (!house_number.empty()) oss << house_number << " ";
    if (!street_name.empty()) oss << street_name << " ";
    if (!street_type.empty()) oss << street_type << " ";
    if (!city.empty()) oss << city << " ";
    if (!state.empty()) oss << state << " ";
    if (!zip_code.empty()) oss << zip_code;
    
    std::string result = oss.str();
    if (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    
    return result;
}

// Geocoder implementation
Geocoder::Geocoder() = default;
Geocoder::~Geocoder() = default;

bool Geocoder::loadAddressData(const std::string& shapefile_path, const std::string& address_field) {
    (void)address_field; // Mark as intentionally unused for now
    ShapefileReader reader(shapefile_path);
    
    if (!reader.open()) {
        return false;
    }
    
    address_data_ = reader.readAllRecords();
    buildIndex();
    
    return !address_data_.empty();
}

GeocodeResult Geocoder::geocode(const std::string& address) const {
    ParsedAddress parsed = parser_.parse(address);
    
    if (!parsed.isValid()) {
        return GeocodeResult();  // Empty result with 0 confidence
    }
    
    std::vector<GeocodeResult> candidates = findCandidates(parsed);
    
    if (candidates.empty()) {
        return GeocodeResult();
    }
    
    // Sort by confidence and return best match
    std::sort(candidates.begin(), candidates.end(), 
              [](const GeocodeResult& a, const GeocodeResult& b) {
                  return a.confidence_score > b.confidence_score;
              });
    
    return candidates[0];
}

std::vector<GeocodeResult> Geocoder::geocodeBatch(const std::vector<std::string>& addresses) const {
    std::vector<GeocodeResult> results;
    results.reserve(addresses.size());
    
    for (const auto& address : addresses) {
        results.push_back(geocode(address));
    }
    
    return results;
}

GeocodeResult Geocoder::reverseGeocode(const Point2D& point, double max_distance) const {
    double min_distance = std::numeric_limits<double>::max();
    GeocodeResult best_result;
    
    for (const auto& record : address_data_) {
        if (!record || !record->geometry) continue;
        
        // Get centroid of geometry for distance calculation
        BoundingBox bounds = record->geometry->getBounds();
        Point2D centroid((bounds.min_x + bounds.max_x) / 2.0, 
                        (bounds.min_y + bounds.max_y) / 2.0);
        
        double distance = calculateDistance(point, centroid);
        
        if (distance <= max_distance && distance < min_distance) {
            min_distance = distance;
            
            // Extract address from record
            std::string address_str = extractAddressFromRecord(*record, "ADDRESS");
            ParsedAddress parsed = parser_.parse(address_str);
            
            best_result = GeocodeResult(centroid, parsed, 1.0 - (distance / max_distance));
            best_result.match_type = "reverse";
        }
    }
    
    return best_result;
}

void Geocoder::buildIndex() {
    index_.street_index.clear();
    index_.city_index.clear();
    index_.zip_index.clear();
    
    for (size_t i = 0; i < address_data_.size(); ++i) {
        const auto& record = address_data_[i];
        if (!record) continue;
        
        std::string address_str = extractAddressFromRecord(*record, "ADDRESS");
        ParsedAddress parsed = parser_.parse(address_str);
        
        // Index by street name
        if (!parsed.street_name.empty()) {
            std::string normalized_street = parser_.normalize(parsed.street_name);
            index_.street_index[normalized_street].push_back(i);
        }
        
        // Index by city
        if (!parsed.city.empty()) {
            std::string normalized_city = parser_.normalize(parsed.city);
            index_.city_index[normalized_city].push_back(i);
        }
        
        // Index by zip code
        if (!parsed.zip_code.empty()) {
            index_.zip_index[parsed.zip_code].push_back(i);
        }
    }
}

std::vector<GeocodeResult> Geocoder::findCandidates(const ParsedAddress& parsed_address) const {
    std::vector<GeocodeResult> candidates;
    std::vector<size_t> candidate_indices;
    
    // Find candidates by street name
    std::string normalized_street = parser_.normalize(parsed_address.street_name);
    auto street_it = index_.street_index.find(normalized_street);
    if (street_it != index_.street_index.end()) {
        candidate_indices.insert(candidate_indices.end(), 
                                street_it->second.begin(), street_it->second.end());
    }
    
    // Remove duplicates
    std::sort(candidate_indices.begin(), candidate_indices.end());
    candidate_indices.erase(std::unique(candidate_indices.begin(), candidate_indices.end()), 
                           candidate_indices.end());
    
    // Evaluate candidates
    for (size_t idx : candidate_indices) {
        if (idx >= address_data_.size() || !address_data_[idx] || !address_data_[idx]->geometry) {
            continue;
        }
        
        const auto& record = address_data_[idx];
        std::string address_str = extractAddressFromRecord(*record, "ADDRESS");
        ParsedAddress candidate_address = parser_.parse(address_str);
        
        double confidence = calculateConfidence(parsed_address, candidate_address);
        
        if (confidence > 0.3) {  // Minimum confidence threshold
            BoundingBox bounds = record->geometry->getBounds();
            Point2D centroid((bounds.min_x + bounds.max_x) / 2.0, 
                           (bounds.min_y + bounds.max_y) / 2.0);
            
            GeocodeResult result(centroid, candidate_address, confidence);
            result.match_type = (confidence > 0.9) ? "exact" : "fuzzy";
            candidates.push_back(result);
        }
    }
    
    return candidates;
}

double Geocoder::calculateConfidence(const ParsedAddress& input, const ParsedAddress& candidate) const {
    double total_score = 0.0;
    int components = 0;
    
    // Street name similarity (highest weight)
    if (!input.street_name.empty() && !candidate.street_name.empty()) {
        double street_similarity = jaroWinklerSimilarity(
            parser_.normalize(input.street_name), 
            parser_.normalize(candidate.street_name)
        );
        total_score += street_similarity * 0.4;
        components++;
    }
    
    // House number match
    if (!input.house_number.empty() && !candidate.house_number.empty()) {
        double house_score = (input.house_number == candidate.house_number) ? 1.0 : 0.0;
        total_score += house_score * 0.3;
        components++;
    }
    
    // City similarity
    if (!input.city.empty() && !candidate.city.empty()) {
        double city_similarity = jaroWinklerSimilarity(
            parser_.normalize(input.city), 
            parser_.normalize(candidate.city)
        );
        total_score += city_similarity * 0.2;
        components++;
    }
    
    // Zip code match
    if (!input.zip_code.empty() && !candidate.zip_code.empty()) {
        double zip_score = (input.zip_code == candidate.zip_code) ? 1.0 : 0.0;
        total_score += zip_score * 0.1;
        components++;
    }
    
    return (components > 0) ? total_score : 0.0;
}

double Geocoder::calculateDistance(const Point2D& p1, const Point2D& p2) const {
    // Simple Euclidean distance - in production, use proper geodesic distance
    double dx = p1.x - p2.x;
    double dy = p1.y - p2.y;
    return std::sqrt(dx * dx + dy * dy);
}

std::string Geocoder::extractAddressFromRecord(const ShapeRecord& record, const std::string& field_name) const {
    auto it = record.attributes.find(field_name);
    if (it != record.attributes.end()) {
        if (std::holds_alternative<std::string>(it->second)) {
            return std::get<std::string>(it->second);
        }
    }
    return "";
}

double Geocoder::jaroWinklerSimilarity(const std::string& s1, const std::string& s2) const {
    // Simplified Jaro-Winkler implementation
    if (s1 == s2) return 1.0;
    if (s1.empty() || s2.empty()) return 0.0;
    
    // This is a simplified version - production code would use full algorithm
    size_t common_chars = 0;
    size_t max_len = std::max(s1.length(), s2.length());
    
    for (size_t i = 0; i < std::min(s1.length(), s2.length()); ++i) {
        if (s1[i] == s2[i]) common_chars++;
    }
    
    return static_cast<double>(common_chars) / max_len;
}

double Geocoder::levenshteinDistance(const std::string& s1, const std::string& s2) const {
    size_t len1 = s1.length();
    size_t len2 = s2.length();
    
    if (len1 == 0) return static_cast<double>(len2);
    if (len2 == 0) return static_cast<double>(len1);
    
    std::vector<std::vector<size_t>> matrix(len1 + 1, std::vector<size_t>(len2 + 1));
    
    for (size_t i = 0; i <= len1; ++i) {
        matrix[i][0] = i;
    }
    for (size_t j = 0; j <= len2; ++j) {
        matrix[0][j] = j;
    }
    
    for (size_t i = 1; i <= len1; ++i) {
        for (size_t j = 1; j <= len2; ++j) {
            size_t cost = (s1[i-1] == s2[j-1]) ? 0 : 1;
            matrix[i][j] = std::min({
                matrix[i-1][j] + 1,      // deletion
                matrix[i][j-1] + 1,      // insertion
                matrix[i-1][j-1] + cost  // substitution
            });
        }
    }
    
    return static_cast<double>(matrix[len1][len2]);
}

std::string Geocoder::getStats() const {
    std::ostringstream oss;
    oss << "Geocoder Statistics:\n";
    oss << "  Total Records: " << address_data_.size() << "\n";
    oss << "  Street Index Entries: " << index_.street_index.size() << "\n";
    oss << "  City Index Entries: " << index_.city_index.size() << "\n";
    oss << "  Zip Index Entries: " << index_.zip_index.size() << "\n";
    return oss.str();
}

} // namespace gis

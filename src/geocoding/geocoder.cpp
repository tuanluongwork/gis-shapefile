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
    
    return normalized;
}

const std::unordered_map<std::string, std::string>& AddressParser::getStateAbbreviations() const {
    return state_abbreviations_;
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
    // For GADM state-level data, we consider an address valid if it has state info
    // or any content in full_address that could be a state query
    return !state.empty() || !full_address.empty();
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
    
    // Build spatial index for efficient point-in-polygon queries
    spatial_index_.buildIndex(address_data_);
    
    return !address_data_.empty();
}

GeocodeResult Geocoder::geocode(const std::string& address) const {
    // First try standard address parsing
    ParsedAddress parsed = parser_.parse(address);
    
    if (parsed.isValid()) {
        std::vector<GeocodeResult> candidates = findCandidates(parsed);
        
        if (!candidates.empty()) {
            // Sort by confidence and return best match
            std::sort(candidates.begin(), candidates.end(), 
                      [](const GeocodeResult& a, const GeocodeResult& b) {
                          return a.confidence_score > b.confidence_score;
                      });
            return candidates[0];
        }
    }
    
    // If no standard address match, try state/place name lookup
    GeocodeResult state_result = geocodeStateName(address);
    if (state_result.confidence_score > 0) {
        return state_result;
    }
    
    return GeocodeResult();  // Empty result with 0 confidence
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
    // First try spatial index for exact point-in-polygon testing
    ShapeRecord* containing_record = spatial_index_.pointInPolygon(point);
    
    if (containing_record) {
        // Extract state name from GADM data (NAME_1 field)
        std::string address_str = extractAddressFromRecord(*containing_record, "NAME_1");
        
        if (!address_str.empty()) {
            ParsedAddress parsed;
            parsed.state = address_str;
            parsed.full_address = address_str;
            
            // Get centroid for result coordinates
            BoundingBox bounds = containing_record->geometry->getBounds();
            Point2D centroid((bounds.min_x + bounds.max_x) / 2.0, 
                            (bounds.min_y + bounds.max_y) / 2.0);
            
            GeocodeResult result(centroid, parsed, 1.0); // High confidence for exact polygon match
            result.match_type = "reverse";
            return result;
        }
    }
    
    // Fallback to distance-based matching if spatial index fails
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
            
            // Extract state name from GADM data (NAME_1 field)
            std::string address_str = extractAddressFromRecord(*record, "NAME_1");
            
            if (!address_str.empty()) {
                ParsedAddress parsed;
                parsed.state = address_str;
                parsed.full_address = address_str;
                
                best_result = GeocodeResult(centroid, parsed, 1.0 - (distance / max_distance));
                best_result.match_type = "reverse";
            }
        }
    }
    
    return best_result;
}

void Geocoder::buildIndex() {
    // Clear existing indices - using only city_index for unified state indexing
    index_.street_index.clear();
    index_.city_index.clear();
    index_.zip_index.clear();
    
    // Build unified state index optimized for GADM administrative boundary data
    for (size_t i = 0; i < address_data_.size(); ++i) {
        const auto& record = address_data_[i];
        if (!record) continue;
        
        // Extract state name from NAME_1 field (primary state name)
        std::string state_name = extractAddressFromRecord(*record, "NAME_1");
        if (!state_name.empty()) {
            // Index by normalized state name for fuzzy matching
            std::string normalized_state = parser_.normalize(state_name);
            index_.city_index[normalized_state].push_back(i);
            
            // Also index the original case for exact matching
            index_.city_index[state_name].push_back(i);
        }
        
        // Index all standard US state abbreviations for this record
        if (!state_name.empty()) {
            // Find matching state abbreviation and index it
            for (const auto& abbrev_pair : parser_.getStateAbbreviations()) {
                if (abbrev_pair.second == parser_.normalize(state_name)) {
                    index_.city_index[abbrev_pair.first].push_back(i);
                    break;
                }
            }
        }
    }
}

std::vector<GeocodeResult> Geocoder::findCandidates(const ParsedAddress& parsed_address) const {
    std::vector<GeocodeResult> candidates;
    std::vector<size_t> candidate_indices;
    
    // Unified state-only lookup strategy using single index
    std::string search_term = parsed_address.state.empty() ? 
                             parsed_address.full_address : parsed_address.state;
    
    if (!search_term.empty()) {
        // Strategy 1: Direct exact match
        auto exact_it = index_.city_index.find(search_term);
        if (exact_it != index_.city_index.end()) {
            candidate_indices.insert(candidate_indices.end(), 
                                    exact_it->second.begin(), exact_it->second.end());
        }
        
        // Strategy 2: Normalized match
        std::string normalized_term = parser_.normalize(search_term);
        auto normalized_it = index_.city_index.find(normalized_term);
        if (normalized_it != index_.city_index.end()) {
            candidate_indices.insert(candidate_indices.end(), 
                                    normalized_it->second.begin(), normalized_it->second.end());
        }
        
        // Strategy 3: State abbreviation expansion (if 2-letter code)
        if (search_term.length() == 2) {
            const auto& abbrevs = parser_.getStateAbbreviations();
            auto abbrev_it = abbrevs.find(parser_.normalize(search_term));
            if (abbrev_it != abbrevs.end()) {
                auto expanded_it = index_.city_index.find(abbrev_it->second);
                if (expanded_it != index_.city_index.end()) {
                    candidate_indices.insert(candidate_indices.end(), 
                                            expanded_it->second.begin(), expanded_it->second.end());
                }
            }
        }
    }
    
    // Remove duplicates and sort
    std::sort(candidate_indices.begin(), candidate_indices.end());
    candidate_indices.erase(std::unique(candidate_indices.begin(), candidate_indices.end()), 
                           candidate_indices.end());
    
    // Evaluate candidates with simplified state-focused confidence calculation
    for (size_t idx : candidate_indices) {
        if (idx >= address_data_.size() || !address_data_[idx] || !address_data_[idx]->geometry) {
            continue;
        }
        
        const auto& record = address_data_[idx];
        
        // Create candidate address from GADM data
        ParsedAddress candidate_address;
        candidate_address.state = extractAddressFromRecord(*record, "NAME_1");
        candidate_address.full_address = candidate_address.state;
        
        // Simplified confidence calculation for state matching
        double confidence = calculateStateConfidence(search_term, candidate_address.state);
        
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
    // Simplified confidence calculation focused on state matching
    return calculateStateConfidence(input.state.empty() ? input.full_address : input.state, 
                                   candidate.state);
}

double Geocoder::calculateStateConfidence(const std::string& input_state, const std::string& candidate_state) const {
    if (input_state.empty() || candidate_state.empty()) {
        return 0.0;
    }
    
    // Exact match (highest confidence)
    if (input_state == candidate_state) {
        return 1.0;
    }
    
    // Normalized exact match
    std::string normalized_input = parser_.normalize(input_state);
    std::string normalized_candidate = parser_.normalize(candidate_state);
    if (normalized_input == normalized_candidate) {
        return 1.0;
    }
    
    // State abbreviation match
    if (input_state.length() == 2) {
        const auto& abbrevs = parser_.getStateAbbreviations();
        auto abbrev_it = abbrevs.find(normalized_input);
        if (abbrev_it != abbrevs.end() && abbrev_it->second == normalized_candidate) {
            return 1.0; // Perfect abbreviation match
        }
    }
    
    // Fuzzy string similarity for partial matches
    double similarity = jaroWinklerSimilarity(normalized_input, normalized_candidate);
    return similarity;
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
    oss << "  Unified State Index Entries: " << index_.city_index.size() << "\n";
    oss << "  Street Index Entries: " << index_.street_index.size() << " (unused)\n";
    oss << "  Zip Index Entries: " << index_.zip_index.size() << " (unused)\n";
    return oss.str();
}

GeocodeResult Geocoder::geocodeStateName(const std::string& query) const {
    // Use unified index for efficient state name lookup
    ParsedAddress parsed;
    parsed.state = query;
    parsed.full_address = query;
    
    std::vector<GeocodeResult> candidates = findCandidates(parsed);
    
    if (!candidates.empty()) {
        // Sort by confidence and return best match
        std::sort(candidates.begin(), candidates.end(), 
                  [](const GeocodeResult& a, const GeocodeResult& b) {
                      return a.confidence_score > b.confidence_score;
                  });
        return candidates[0];
    }
    
    return GeocodeResult(); // Empty result
}

} // namespace gis

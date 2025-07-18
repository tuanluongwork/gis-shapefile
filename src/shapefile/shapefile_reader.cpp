#include "gis/shapefile_reader.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <chrono>

namespace gis {

ShapefileReader::ShapefileReader(const std::string& filename)
    : base_filename_(filename)
    , file_code_(0)
    , file_length_(0)
    , version_(0)
    , shape_type_(ShapeType::NullShape)
    , record_count_(0)
    , header_length_(0)
    , record_length_(0)
    , is_open_(false) {
}

ShapefileReader::~ShapefileReader() {
    close();
}

bool ShapefileReader::open() {
    // Open .shp file
    std::string shp_filename = base_filename_ + ".shp";
    shp_file_.open(shp_filename, std::ios::binary);
    if (!shp_file_.is_open()) {
        std::cerr << "Failed to open " << shp_filename << std::endl;
        return false;
    }

    // Open .shx file
    std::string shx_filename = base_filename_ + ".shx";
    shx_file_.open(shx_filename, std::ios::binary);
    if (!shx_file_.is_open()) {
        std::cerr << "Failed to open " << shx_filename << std::endl;
        return false;
    }

    // Open .dbf file (optional)
    std::string dbf_filename = base_filename_ + ".dbf";
    dbf_file_.open(dbf_filename, std::ios::binary);
    
    // Read headers
    if (!readShapefileHeader()) {
        std::cerr << "Failed to read shapefile header" << std::endl;
        return false;
    }
    
    if (dbf_file_.is_open() && !readDBFHeader()) {
        std::cerr << "Failed to read DBF header" << std::endl;
        return false;
    }
    
    is_open_ = true;
    return true;
}

void ShapefileReader::close() {
    if (shp_file_.is_open()) shp_file_.close();
    if (shx_file_.is_open()) shx_file_.close();
    if (dbf_file_.is_open()) dbf_file_.close();
    is_open_ = false;
}

bool ShapefileReader::readShapefileHeader() {
    shp_file_.seekg(0);
    
    // Read main file header (100 bytes)
    file_code_ = readValue<int32_t>(shp_file_, true);  // Big endian
    if (file_code_ != 9994) {
        std::cerr << "Invalid shapefile file code: " << file_code_ << std::endl;
        return false;
    }
    
    // Skip unused fields (5 * 4 bytes)
    shp_file_.seekg(20, std::ios::cur);
    
    file_length_ = readValue<int32_t>(shp_file_, true);  // Big endian, in 16-bit words
    version_ = readValue<int32_t>(shp_file_);            // Little endian
    shape_type_ = static_cast<ShapeType>(readValue<int32_t>(shp_file_));
    
    // Read bounding box
    bounds_.min_x = readValue<double>(shp_file_);
    bounds_.min_y = readValue<double>(shp_file_);
    bounds_.max_x = readValue<double>(shp_file_);
    bounds_.max_y = readValue<double>(shp_file_);
    
    // Skip Z and M ranges (4 * 8 bytes)
    shp_file_.seekg(32, std::ios::cur);
    
    return true;
}

bool ShapefileReader::readDBFHeader() {
    if (!dbf_file_.is_open()) return false;
    
    dbf_file_.seekg(0);
    
    // Read DBF header
    uint8_t version = readValue<uint8_t>(dbf_file_);
    
    // Skip date (3 bytes)
    dbf_file_.seekg(3, std::ios::cur);
    
    record_count_ = readValue<uint32_t>(dbf_file_);
    header_length_ = readValue<uint16_t>(dbf_file_);
    record_length_ = readValue<uint16_t>(dbf_file_);
    
    // Skip reserved fields (20 bytes)
    dbf_file_.seekg(20, std::ios::cur);
    
    // Read field definitions
    field_definitions_.clear();
    size_t field_offset = 32;  // DBF header size
    
    while (field_offset < header_length_ - 1) {  // -1 for field terminator
        FieldDefinition field;
        
        // Read field name (11 bytes, null-terminated)
        char field_name[12] = {0};
        dbf_file_.read(field_name, 11);
        field.name = std::string(field_name);
        
        // Read field type
        char type_char = readValue<char>(dbf_file_);
        switch (type_char) {
            case 'C': field.type = FieldType::Character; break;
            case 'N': field.type = FieldType::Numeric; break;
            case 'L': field.type = FieldType::Logical; break;
            case 'D': field.type = FieldType::Date; break;
            case 'F': field.type = FieldType::Float; break;
            default: field.type = FieldType::Unknown; break;
        }
        
        // Skip field data address (4 bytes)
        dbf_file_.seekg(4, std::ios::cur);
        
        field.length = readValue<uint8_t>(dbf_file_);
        field.decimal_count = readValue<uint8_t>(dbf_file_);
        
        // Skip reserved fields (14 bytes)
        dbf_file_.seekg(14, std::ios::cur);
        
        field_definitions_.push_back(field);
        field_offset += 32;
    }
    
    return true;
}

std::unique_ptr<ShapeRecord> ShapefileReader::readRecord(uint32_t index) {
    if (!is_open_ || index >= record_count_) {
        return nullptr;
    }
    
    // Read from .shx file to get record offset and length
    shx_file_.seekg(100 + index * 8);  // Skip header + index * record_size
    int32_t offset = readValue<int32_t>(shx_file_, true) * 2;  // Convert from words to bytes
    int32_t length = readValue<int32_t>(shx_file_, true) * 2;
    
    // Read shape record from .shp file
    shp_file_.seekg(offset);
    int32_t record_number = readValue<int32_t>(shp_file_, true);
    int32_t content_length = readValue<int32_t>(shp_file_, true);
    
    auto record = std::make_unique<ShapeRecord>();
    record->record_number = record_number;
    
    // Read geometry
    ShapeType record_shape_type = static_cast<ShapeType>(readValue<int32_t>(shp_file_));
    if (record_shape_type != ShapeType::NullShape) {
        record->geometry = readGeometry(shp_file_, record_shape_type);
    }
    
    // Read DBF attributes
    if (dbf_file_.is_open()) {
        record->attributes = readDBFRecord(index);
    }
    
    return record;
}

std::vector<std::unique_ptr<ShapeRecord>> ShapefileReader::readAllRecords() {
    std::vector<std::unique_ptr<ShapeRecord>> records;
    records.reserve(record_count_);
    
    for (uint32_t i = 0; i < record_count_; ++i) {
        auto record = readRecord(i);
        if (record) {
            records.push_back(std::move(record));
        }
    }
    
    return records;
}

std::vector<std::unique_ptr<ShapeRecord>> ShapefileReader::readRecordsInBounds(const BoundingBox& bbox) {
    std::vector<std::unique_ptr<ShapeRecord>> records;
    
    if (!is_open_) return records;
    
    // Read all records and filter by bounding box
    for (uint32_t i = 0; i < record_count_; ++i) {
        auto record = readRecord(i);
        if (record && record->geometry) {
            BoundingBox record_bounds = record->geometry->getBounds();
            
            // Check if the record's bounding box intersects with the query bbox
            if (bbox.intersects(record_bounds)) {
                records.push_back(std::move(record));
            }
        }
    }
    
    return records;
}

std::unique_ptr<Geometry> ShapefileReader::readGeometry(std::ifstream& file, ShapeType type) {
    switch (type) {
        case ShapeType::Point:
            return readPoint(file);
        case ShapeType::PolyLine:
            return readPolyline(file);
        case ShapeType::Polygon:
            return readPolygon(file);
        default:
            // Skip unsupported geometry types
            return nullptr;
    }
}

std::unique_ptr<PointGeometry> ShapefileReader::readPoint(std::ifstream& file) {
    double x = readValue<double>(file);
    double y = readValue<double>(file);
    return std::make_unique<PointGeometry>(Point2D(x, y));
}

std::unique_ptr<PolylineGeometry> ShapefileReader::readPolyline(std::ifstream& file) {
    // Skip bounding box (4 * 8 bytes)
    file.seekg(32, std::ios::cur);
    
    int32_t num_parts = readValue<int32_t>(file);
    int32_t num_points = readValue<int32_t>(file);
    
    // Read part indices
    std::vector<int32_t> parts(num_parts);
    for (int32_t i = 0; i < num_parts; ++i) {
        parts[i] = readValue<int32_t>(file);
    }
    
    // Read all points
    std::vector<Point2D> all_points(num_points);
    for (int32_t i = 0; i < num_points; ++i) {
        all_points[i].x = readValue<double>(file);
        all_points[i].y = readValue<double>(file);
    }
    
    // Split points into parts
    std::vector<std::vector<Point2D>> polyline_parts;
    for (int32_t i = 0; i < num_parts; ++i) {
        int32_t start = parts[i];
        int32_t end = (i == num_parts - 1) ? num_points : parts[i + 1];
        
        std::vector<Point2D> part(all_points.begin() + start, all_points.begin() + end);
        polyline_parts.push_back(std::move(part));
    }
    
    return std::make_unique<PolylineGeometry>(std::move(polyline_parts));
}

std::unique_ptr<PolygonGeometry> ShapefileReader::readPolygon(std::ifstream& file) {
    // Skip bounding box (4 * 8 bytes)
    file.seekg(32, std::ios::cur);
    
    int32_t num_parts = readValue<int32_t>(file);
    int32_t num_points = readValue<int32_t>(file);
    
    // Read part indices
    std::vector<int32_t> parts(num_parts);
    for (int32_t i = 0; i < num_parts; ++i) {
        parts[i] = readValue<int32_t>(file);
    }
    
    // Read all points
    std::vector<Point2D> all_points(num_points);
    for (int32_t i = 0; i < num_points; ++i) {
        all_points[i].x = readValue<double>(file);
        all_points[i].y = readValue<double>(file);
    }
    
    // Split points into rings
    std::vector<std::vector<Point2D>> rings;
    for (int32_t i = 0; i < num_parts; ++i) {
        int32_t start = parts[i];
        int32_t end = (i == num_parts - 1) ? num_points : parts[i + 1];
        
        std::vector<Point2D> ring(all_points.begin() + start, all_points.begin() + end);
        rings.push_back(std::move(ring));
    }
    
    return std::make_unique<PolygonGeometry>(std::move(rings));
}

std::unordered_map<std::string, FieldValue> ShapefileReader::readDBFRecord(uint32_t record_index) {
    std::unordered_map<std::string, FieldValue> attributes;
    
    if (!dbf_file_.is_open() || record_index >= record_count_) {
        return attributes;
    }
    
    // Calculate record position
    size_t record_pos = header_length_ + record_index * record_length_;
    dbf_file_.seekg(record_pos);
    
    // Skip deletion flag
    char deletion_flag = readValue<char>(dbf_file_);
    if (deletion_flag == '*') {
        return attributes;  // Record is deleted
    }
    
    // Read field values
    for (const auto& field : field_definitions_) {
        std::vector<char> field_data(field.length);
        dbf_file_.read(field_data.data(), field.length);
        
        std::string field_str(field_data.begin(), field_data.end());
        
        // Trim whitespace
        field_str.erase(0, field_str.find_first_not_of(" \t"));
        field_str.erase(field_str.find_last_not_of(" \t") + 1);
        
        // Convert based on field type
        switch (field.type) {
            case FieldType::Character:
                attributes[field.name] = field_str;
                break;
            case FieldType::Numeric:
            case FieldType::Float:
                if (!field_str.empty()) {
                    try {
                        attributes[field.name] = std::stod(field_str);
                    } catch (...) {
                        attributes[field.name] = 0.0;
                    }
                } else {
                    attributes[field.name] = 0.0;
                }
                break;
            case FieldType::Logical:
                attributes[field.name] = (field_str == "T" || field_str == "t" || field_str == "Y" || field_str == "y");
                break;
            default:
                attributes[field.name] = field_str;
                break;
        }
    }
    
    return attributes;
}

template<typename T>
T ShapefileReader::readValue(std::ifstream& file, bool swap_endian) {
    T value;
    file.read(reinterpret_cast<char*>(&value), sizeof(T));
    
    if (swap_endian && sizeof(T) > 1) {
        swapEndian(&value, sizeof(T));
    }
    
    return value;
}

void ShapefileReader::swapEndian(void* data, size_t size) {
    char* bytes = static_cast<char*>(data);
    for (size_t i = 0; i < size / 2; ++i) {
        std::swap(bytes[i], bytes[size - 1 - i]);
    }
}

std::string ShapefileReader::getInfo() const {
    std::ostringstream oss;
    oss << "Shapefile Information:\n";
    oss << "  File: " << base_filename_ << "\n";
    oss << "  Shape Type: " << static_cast<int>(shape_type_) << "\n";
    oss << "  Record Count: " << record_count_ << "\n";
    oss << "  Bounds: (" << bounds_.min_x << ", " << bounds_.min_y 
        << ") to (" << bounds_.max_x << ", " << bounds_.max_y << ")\n";
    
    if (!field_definitions_.empty()) {
        oss << "  Fields:\n";
        for (const auto& field : field_definitions_) {
            oss << "    " << field.name << " (" << static_cast<int>(field.type) 
                << ", " << static_cast<int>(field.length) << ")\n";
        }
    }
    
    return oss.str();
}

} // namespace gis

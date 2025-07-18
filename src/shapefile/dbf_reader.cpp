#include "gis/dbf_reader.h"
#include <iostream>
#include <algorithm>

namespace gis {

DBFReader::DBFReader(const std::string& filename) 
    : record_count_(0), header_length_(0), record_length_(0), is_open_(false) {
    file_.open(filename + ".dbf", std::ios::binary);
}

DBFReader::~DBFReader() {
    close();
}

bool DBFReader::open() {
    if (!file_.is_open()) {
        return false;
    }
    
    if (!readHeader()) {
        return false;
    }
    
    is_open_ = true;
    return true;
}

void DBFReader::close() {
    if (file_.is_open()) {
        file_.close();
    }
    is_open_ = false;
}

bool DBFReader::readHeader() {
    file_.seekg(0);
    
    // Read DBF header (32 bytes)
    uint8_t version = readValue<uint8_t>();
    
    // Skip date (3 bytes)
    file_.seekg(3, std::ios::cur);
    
    record_count_ = readValue<uint32_t>();
    header_length_ = readValue<uint16_t>();
    record_length_ = readValue<uint16_t>();
    
    // Skip reserved fields (20 bytes)
    file_.seekg(20, std::ios::cur);
    
    // Read field definitions
    fields_.clear();
    size_t field_offset = 32;
    
    while (field_offset < header_length_ - 1) {
        FieldDefinition field;
        
        // Read field name (11 bytes)
        char field_name[12] = {0};
        file_.read(field_name, 11);
        field.name = std::string(field_name);
        
        // Read field type
        char type_char = readValue<char>();
        switch (type_char) {
            case 'C': field.type = FieldType::Character; break;
            case 'N': field.type = FieldType::Numeric; break;
            case 'L': field.type = FieldType::Logical; break;
            case 'D': field.type = FieldType::Date; break;
            case 'F': field.type = FieldType::Float; break;
            default: field.type = FieldType::Unknown; break;
        }
        
        // Skip field data address (4 bytes)
        file_.seekg(4, std::ios::cur);
        
        field.length = readValue<uint8_t>();
        field.decimal_count = readValue<uint8_t>();
        
        // Skip reserved fields (14 bytes)
        file_.seekg(14, std::ios::cur);
        
        fields_.push_back(field);
        field_offset += 32;
    }
    
    return true;
}

std::unordered_map<std::string, FieldValue> DBFReader::readRecord(uint32_t index) {
    std::unordered_map<std::string, FieldValue> record;
    
    if (!is_open_ || index >= record_count_) {
        return record;
    }
    
    // Calculate record position
    size_t record_pos = header_length_ + index * record_length_;
    file_.seekg(record_pos);
    
    // Skip deletion flag
    char deletion_flag = readValue<char>();
    if (deletion_flag == '*') {
        return record; // Record is deleted
    }
    
    // Read field values
    for (const auto& field : fields_) {
        std::vector<char> field_data(field.length);
        file_.read(field_data.data(), field.length);
        
        std::string field_str(field_data.begin(), field_data.end());
        FieldValue value = parseFieldValue(field_str, field);
        record[field.name] = value;
    }
    
    return record;
}

FieldValue DBFReader::parseFieldValue(const std::string& data, const FieldDefinition& field) {
    // Trim whitespace
    std::string trimmed = data;
    trimmed.erase(0, trimmed.find_first_not_of(" \t"));
    trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
    
    switch (field.type) {
        case FieldType::Character:
            return trimmed;
            
        case FieldType::Numeric:
        case FieldType::Float:
            if (!trimmed.empty()) {
                try {
                    return std::stod(trimmed);
                } catch (...) {
                    return 0.0;
                }
            }
            return 0.0;
            
        case FieldType::Logical:
            return (trimmed == "T" || trimmed == "t" || trimmed == "Y" || trimmed == "y");
            
        default:
            return trimmed;
    }
}

template<typename T>
T DBFReader::readValue() {
    T value;
    file_.read(reinterpret_cast<char*>(&value), sizeof(T));
    return value;
}

} // namespace gis

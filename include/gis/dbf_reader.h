#include "gis/shapefile_reader.h"

namespace gis {

/**
 * @brief DBF (dBase) file reader implementation
 * 
 * Handles reading attribute data from DBF files associated with shapefiles.
 * Supports various field types including character, numeric, logical, and date fields.
 */
class DBFReader {
private:
    std::ifstream file_;
    std::vector<FieldDefinition> fields_;
    uint32_t record_count_;
    uint16_t header_length_;
    uint16_t record_length_;
    bool is_open_;
    
public:
    explicit DBFReader(const std::string& filename);
    ~DBFReader();
    
    bool open();
    void close();
    bool isOpen() const { return is_open_; }
    
    uint32_t getRecordCount() const { return record_count_; }
    const std::vector<FieldDefinition>& getFields() const { return fields_; }
    
    std::unordered_map<std::string, FieldValue> readRecord(uint32_t index);
    
private:
    bool readHeader();
    FieldValue parseFieldValue(const std::string& data, const FieldDefinition& field);
    
    template<typename T>
    T readValue();
};

} // namespace gis

#pragma once

#include "geometry.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <variant>

namespace gis {

/**
 * @brief Supported field types in DBF files
 */
enum class FieldType {
    Character,
    Numeric,
    Logical,
    Date,
    Float,
    Unknown
};

/**
 * @brief Represents a field value from DBF record
 */
using FieldValue = std::variant<std::string, double, bool, int>;

/**
 * @brief Shapefile record containing geometry and attributes
 */
struct ShapeRecord {
    int32_t record_number;
    std::unique_ptr<Geometry> geometry;
    std::unordered_map<std::string, FieldValue> attributes;
    
    ShapeRecord() : record_number(0) {}
    ShapeRecord(ShapeRecord&& other) noexcept 
        : record_number(other.record_number)
        , geometry(std::move(other.geometry))
        , attributes(std::move(other.attributes)) {}
    
    ShapeRecord& operator=(ShapeRecord&& other) noexcept {
        if (this != &other) {
            record_number = other.record_number;
            geometry = std::move(other.geometry);
            attributes = std::move(other.attributes);
        }
        return *this;
    }
    
    // Delete copy constructor and assignment
    ShapeRecord(const ShapeRecord&) = delete;
    ShapeRecord& operator=(const ShapeRecord&) = delete;
};

/**
 * @brief Field definition from DBF header
 */
struct FieldDefinition {
    std::string name;
    FieldType type;
    uint8_t length;
    uint8_t decimal_count;
};

/**
 * @brief Main class for reading ESRI Shapefiles
 * 
 * This class provides a complete implementation for reading shapefiles (.shp),
 * index files (.shx), and database files (.dbf). It supports all standard
 * shapefile geometry types and provides efficient access to spatial data.
 */
class ShapefileReader {
private:
    std::string base_filename_;
    std::ifstream shp_file_;
    std::ifstream shx_file_;
    std::ifstream dbf_file_;
    
    // Header information
    int32_t file_code_;
    int32_t file_length_;
    int32_t version_;
    ShapeType shape_type_;
    BoundingBox bounds_;
    
    // DBF information
    std::vector<FieldDefinition> field_definitions_;
    uint32_t record_count_;
    uint16_t header_length_;
    uint16_t record_length_;
    
    bool is_open_;
    
public:
    /**
     * @brief Constructor
     * @param filename Base filename without extension (e.g., "data/cities")
     */
    explicit ShapefileReader(const std::string& filename);
    
    /**
     * @brief Destructor
     */
    ~ShapefileReader();
    
    /**
     * @brief Open the shapefile and associated files
     * @return true if successful, false otherwise
     */
    bool open();
    
    /**
     * @brief Close all open files
     */
    void close();
    
    /**
     * @brief Check if the shapefile is open and ready for reading
     * @return true if open, false otherwise
     */
    bool isOpen() const { return is_open_; }
    
    /**
     * @brief Get the number of records in the shapefile
     * @return Number of records
     */
    uint32_t getRecordCount() const { return record_count_; }
    
    /**
     * @brief Get the shape type of the shapefile
     * @return Shape type
     */
    ShapeType getShapeType() const { return shape_type_; }
    
    /**
     * @brief Get the bounding box of all shapes
     * @return Bounding box
     */
    const BoundingBox& getBounds() const { return bounds_; }
    
    /**
     * @brief Get field definitions from the DBF file
     * @return Vector of field definitions
     */
    const std::vector<FieldDefinition>& getFieldDefinitions() const { 
        return field_definitions_; 
    }
    
    /**
     * @brief Read a specific record by index
     * @param index Zero-based record index
     * @return Unique pointer to shape record, or nullptr if error
     */
    std::unique_ptr<ShapeRecord> readRecord(uint32_t index);
    
    /**
     * @brief Read all records from the shapefile
     * @return Vector of shape records
     */
    std::vector<std::unique_ptr<ShapeRecord>> readAllRecords();
    
    /**
     * @brief Read records within a bounding box
     * @param bbox Bounding box to filter records
     * @return Vector of shape records that intersect the bbox
     */
    std::vector<std::unique_ptr<ShapeRecord>> readRecordsInBounds(const BoundingBox& bbox);
    
    /**
     * @brief Get detailed information about the shapefile
     * @return String containing shapefile metadata
     */
    std::string getInfo() const;

private:
    bool readShapefileHeader();
    bool readDBFHeader();
    std::unique_ptr<Geometry> readGeometry(std::ifstream& file, ShapeType type);
    std::unique_ptr<PointGeometry> readPoint(std::ifstream& file);
    std::unique_ptr<PolylineGeometry> readPolyline(std::ifstream& file);
    std::unique_ptr<PolygonGeometry> readPolygon(std::ifstream& file);
    std::unordered_map<std::string, FieldValue> readDBFRecord(uint32_t record_index);
    
    template<typename T>
    T readValue(std::ifstream& file, bool swap_endian = false);
    
    void swapEndian(void* data, size_t size);
};

} // namespace gis

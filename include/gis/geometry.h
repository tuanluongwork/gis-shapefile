#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <cstdint>

namespace gis {

/**
 * @brief Represents different geometry types in shapefiles
 */
enum class ShapeType : int32_t {
    NullShape = 0,
    Point = 1,
    PolyLine = 3,
    Polygon = 5,
    MultiPoint = 8,
    PointZ = 11,
    PolyLineZ = 13,
    PolygonZ = 15,
    MultiPointZ = 18,
    PointM = 21,
    PolyLineM = 23,
    PolygonM = 25,
    MultiPointM = 28,
    MultiPatch = 31
};

/**
 * @brief Basic 2D point structure
 */
struct Point2D {
    double x, y;
    
    Point2D() : x(0.0), y(0.0) {}
    Point2D(double x_, double y_) : x(x_), y(y_) {}
    
    bool operator==(const Point2D& other) const {
        return std::abs(x - other.x) < 1e-9 && std::abs(y - other.y) < 1e-9;
    }
};

/**
 * @brief 3D point with Z coordinate
 */
struct Point3D : Point2D {
    double z;
    
    Point3D() : Point2D(), z(0.0) {}
    Point3D(double x_, double y_, double z_) : Point2D(x_, y_), z(z_) {}
};

/**
 * @brief Bounding box for spatial extents
 */
struct BoundingBox {
    double min_x, min_y, max_x, max_y;
    
    BoundingBox() : min_x(0), min_y(0), max_x(0), max_y(0) {}
    BoundingBox(double min_x_, double min_y_, double max_x_, double max_y_) 
        : min_x(min_x_), min_y(min_y_), max_x(max_x_), max_y(max_y_) {}
    
    bool contains(const Point2D& point) const;
    bool intersects(const BoundingBox& other) const;
    double area() const { return (max_x - min_x) * (max_y - min_y); }
};

/**
 * @brief Base class for all geometry types
 */
class Geometry {
public:
    virtual ~Geometry() = default;
    virtual ShapeType getType() const = 0;
    virtual BoundingBox getBounds() const = 0;
    virtual std::unique_ptr<Geometry> clone() const = 0;
};

/**
 * @brief Point geometry implementation
 */
class PointGeometry : public Geometry {
private:
    Point2D point_;
    
public:
    explicit PointGeometry(const Point2D& point) : point_(point) {}
    
    ShapeType getType() const override { return ShapeType::Point; }
    BoundingBox getBounds() const override;
    std::unique_ptr<Geometry> clone() const override;
    
    const Point2D& getPoint() const { return point_; }
};

/**
 * @brief Polyline geometry (series of connected line segments)
 */
class PolylineGeometry : public Geometry {
private:
    std::vector<std::vector<Point2D>> parts_;
    
public:
    explicit PolylineGeometry(std::vector<std::vector<Point2D>> parts) 
        : parts_(std::move(parts)) {}
    
    ShapeType getType() const override { return ShapeType::PolyLine; }
    BoundingBox getBounds() const override;
    std::unique_ptr<Geometry> clone() const override;
    
    const std::vector<std::vector<Point2D>>& getParts() const { return parts_; }
    size_t getNumParts() const { return parts_.size(); }
};

/**
 * @brief Polygon geometry with support for holes
 */
class PolygonGeometry : public Geometry {
private:
    std::vector<std::vector<Point2D>> rings_;
    
public:
    explicit PolygonGeometry(std::vector<std::vector<Point2D>> rings) 
        : rings_(std::move(rings)) {}
    
    ShapeType getType() const override { return ShapeType::Polygon; }
    BoundingBox getBounds() const override;
    std::unique_ptr<Geometry> clone() const override;
    
    const std::vector<std::vector<Point2D>>& getRings() const { return rings_; }
    size_t getNumRings() const { return rings_.size(); }
    
    /**
     * @brief Check if a point is inside the polygon
     * @param point The point to test
     * @return true if point is inside the polygon
     */
    bool contains(const Point2D& point) const;
};

} // namespace gis

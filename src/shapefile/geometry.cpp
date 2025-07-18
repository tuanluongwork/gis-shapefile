#include "gis/geometry.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace gis {

// BoundingBox methods
bool BoundingBox::contains(const Point2D& point) const {
    return point.x >= min_x && point.x <= max_x && 
           point.y >= min_y && point.y <= max_y;
}

bool BoundingBox::intersects(const BoundingBox& other) const {
    return !(other.min_x > max_x || other.max_x < min_x ||
             other.min_y > max_y || other.max_y < min_y);
}

// PointGeometry methods
BoundingBox PointGeometry::getBounds() const {
    return BoundingBox(point_.x, point_.y, point_.x, point_.y);
}

std::unique_ptr<Geometry> PointGeometry::clone() const {
    return std::make_unique<PointGeometry>(point_);
}

// PolylineGeometry methods
BoundingBox PolylineGeometry::getBounds() const {
    if (parts_.empty()) {
        return BoundingBox();
    }
    
    double min_x = std::numeric_limits<double>::max();
    double min_y = std::numeric_limits<double>::max();
    double max_x = std::numeric_limits<double>::lowest();
    double max_y = std::numeric_limits<double>::lowest();
    
    for (const auto& part : parts_) {
        for (const auto& point : part) {
            min_x = std::min(min_x, point.x);
            min_y = std::min(min_y, point.y);
            max_x = std::max(max_x, point.x);
            max_y = std::max(max_y, point.y);
        }
    }
    
    return BoundingBox(min_x, min_y, max_x, max_y);
}

std::unique_ptr<Geometry> PolylineGeometry::clone() const {
    return std::make_unique<PolylineGeometry>(parts_);
}

// PolygonGeometry methods
BoundingBox PolygonGeometry::getBounds() const {
    if (rings_.empty()) {
        return BoundingBox();
    }
    
    double min_x = std::numeric_limits<double>::max();
    double min_y = std::numeric_limits<double>::max();
    double max_x = std::numeric_limits<double>::lowest();
    double max_y = std::numeric_limits<double>::lowest();
    
    for (const auto& ring : rings_) {
        for (const auto& point : ring) {
            min_x = std::min(min_x, point.x);
            min_y = std::min(min_y, point.y);
            max_x = std::max(max_x, point.x);
            max_y = std::max(max_y, point.y);
        }
    }
    
    return BoundingBox(min_x, min_y, max_x, max_y);
}

std::unique_ptr<Geometry> PolygonGeometry::clone() const {
    return std::make_unique<PolygonGeometry>(rings_);
}

bool PolygonGeometry::contains(const Point2D& point) const {
    if (rings_.empty()) {
        return false;
    }
    
    // Point-in-polygon test using ray casting algorithm
    auto pointInRing = [](const std::vector<Point2D>& ring, const Point2D& point) -> bool {
        bool inside = false;
        size_t j = ring.size() - 1;
        
        for (size_t i = 0; i < ring.size(); i++) {
            const Point2D& pi = ring[i];
            const Point2D& pj = ring[j];
            
            if (((pi.y > point.y) != (pj.y > point.y)) &&
                (point.x < (pj.x - pi.x) * (point.y - pi.y) / (pj.y - pi.y) + pi.x)) {
                inside = !inside;
            }
            j = i;
        }
        
        return inside;
    };
    
    // Check if point is in outer ring (first ring)
    if (!pointInRing(rings_[0], point)) {
        return false;
    }
    
    // Check if point is in any hole (subsequent rings)
    for (size_t i = 1; i < rings_.size(); i++) {
        if (pointInRing(rings_[i], point)) {
            return false;  // Point is in a hole
        }
    }
    
    return true;
}

} // namespace gis

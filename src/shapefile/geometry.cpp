#include "gis/geometry.h"
#include "logger.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace gis {

// BoundingBox methods
bool BoundingBox::contains(const Point2D& point) const {
    bool result = point.x >= min_x && point.x <= max_x && 
                  point.y >= min_y && point.y <= max_y;
    
    LOG_DEBUG("BoundingBox", "Point containment check", 
             {{"point_x", std::to_string(point.x)}, 
              {"point_y", std::to_string(point.y)},
              {"bbox_min_x", std::to_string(min_x)},
              {"bbox_min_y", std::to_string(min_y)},
              {"bbox_max_x", std::to_string(max_x)},
              {"bbox_max_y", std::to_string(max_y)},
              {"contains", result ? "true" : "false"}});
    
    return result;
}

bool BoundingBox::intersects(const BoundingBox& other) const {
    bool result = !(other.min_x > max_x || other.max_x < min_x ||
                    other.min_y > max_y || other.max_y < min_y);
    
    LOG_DEBUG("BoundingBox", "Intersection check", 
             {{"bbox1_bounds", std::to_string(min_x) + "," + std::to_string(min_y) + "," + 
                               std::to_string(max_x) + "," + std::to_string(max_y)},
              {"bbox2_bounds", std::to_string(other.min_x) + "," + std::to_string(other.min_y) + "," + 
                               std::to_string(other.max_x) + "," + std::to_string(other.max_y)},
              {"intersects", result ? "true" : "false"}});
    
    return result;
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
        LOG_DEBUG("PolylineGeometry", "Getting bounds for empty polyline", {});
        return BoundingBox();
    }
    
    size_t total_points = 0;
    for (const auto& part : parts_) {
        total_points += part.size();
    }
    
    LOG_DEBUG("PolylineGeometry", "Calculating bounds for polyline", 
             {{"parts_count", std::to_string(parts_.size())}, 
              {"total_points", std::to_string(total_points)}});
    
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
    
    BoundingBox result(min_x, min_y, max_x, max_y);
    LOG_DEBUG("PolylineGeometry", "Polyline bounds calculated", 
             {{"bounds", std::to_string(min_x) + "," + std::to_string(min_y) + "," + 
                        std::to_string(max_x) + "," + std::to_string(max_y)}});
    
    return result;
}

std::unique_ptr<Geometry> PolylineGeometry::clone() const {
    return std::make_unique<PolylineGeometry>(parts_);
}

// PolygonGeometry methods
BoundingBox PolygonGeometry::getBounds() const {
    if (rings_.empty()) {
        LOG_DEBUG("PolygonGeometry", "Getting bounds for empty polygon", {});
        return BoundingBox();
    }
    
    size_t total_points = 0;
    for (const auto& ring : rings_) {
        total_points += ring.size();
    }
    
    LOG_DEBUG("PolygonGeometry", "Calculating bounds for polygon", 
             {{"rings_count", std::to_string(rings_.size())}, 
              {"total_points", std::to_string(total_points)}});
    
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
    
    BoundingBox result(min_x, min_y, max_x, max_y);
    LOG_DEBUG("PolygonGeometry", "Polygon bounds calculated", 
             {{"bounds", std::to_string(min_x) + "," + std::to_string(min_y) + "," + 
                        std::to_string(max_x) + "," + std::to_string(max_y)}});
    
    return result;
}

std::unique_ptr<Geometry> PolygonGeometry::clone() const {
    return std::make_unique<PolygonGeometry>(rings_);
}

bool PolygonGeometry::contains(const Point2D& point) const {
    if (rings_.empty()) {
        LOG_DEBUG("PolygonGeometry", "Point containment check on empty polygon", 
                 {{"point_x", std::to_string(point.x)}, 
                  {"point_y", std::to_string(point.y)}});
        return false;
    }
    
    LOG_DEBUG("PolygonGeometry", "Starting point-in-polygon test", 
             {{"point_x", std::to_string(point.x)}, 
              {"point_y", std::to_string(point.y)},
              {"rings_count", std::to_string(rings_.size())}});
    
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
    bool in_outer_ring = pointInRing(rings_[0], point);
    if (!in_outer_ring) {
        LOG_DEBUG("PolygonGeometry", "Point not in outer ring", 
                 {{"point_x", std::to_string(point.x)}, 
                  {"point_y", std::to_string(point.y)}});
        return false;
    }
    
    // Check if point is in any hole (subsequent rings)
    for (size_t i = 1; i < rings_.size(); i++) {
        if (pointInRing(rings_[i], point)) {
            LOG_DEBUG("PolygonGeometry", "Point is in hole", 
                     {{"point_x", std::to_string(point.x)}, 
                      {"point_y", std::to_string(point.y)},
                      {"hole_index", std::to_string(i)}});
            return false;  // Point is in a hole
        }
    }
    
    LOG_DEBUG("PolygonGeometry", "Point containment result", 
             {{"point_x", std::to_string(point.x)}, 
              {"point_y", std::to_string(point.y)},
              {"contains", "true"}});
    
    return true;
}

} // namespace gis

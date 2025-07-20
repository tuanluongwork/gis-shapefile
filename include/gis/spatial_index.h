#pragma once

#include "geometry.h"
#include "shapefile_reader.h"
#include <vector>
#include <memory>
#include <functional>

namespace gis {

/**
 * @brief Node in R-tree spatial index
 */
struct RTreeNode {
    BoundingBox bounds;
    bool is_leaf;
    std::vector<std::unique_ptr<RTreeNode>> children;
    std::vector<size_t> data_indices;  // For leaf nodes
    RTreeNode* parent;  // Parent pointer for upward propagation
    
    RTreeNode(bool leaf = false) : is_leaf(leaf), parent(nullptr) {}
};

/**
 * @brief R-tree spatial index for efficient spatial queries
 * 
 * Implements the R-tree data structure for indexing geometric objects
 * in multi-dimensional space. Optimized for 2D spatial queries.
 */
class RTree {
private:
    std::unique_ptr<RTreeNode> root_;
    size_t max_entries_;
    size_t min_entries_;
    std::vector<BoundingBox> object_bounds_;
    
public:
    /**
     * @brief Constructor
     * @param max_entries Maximum entries per node (default: 16)
     */
    explicit RTree(size_t max_entries = 16);
    
    /**
     * @brief Destructor
     */
    ~RTree();
    
    /**
     * @brief Insert an object into the index
     * @param bounds Bounding box of the object
     * @param data_index Index of the object in external data structure
     */
    void insert(const BoundingBox& bounds, size_t data_index);
    
    /**
     * @brief Query objects that intersect with given bounding box
     * @param query_bounds Query bounding box
     * @return Vector of data indices of intersecting objects
     */
    std::vector<size_t> query(const BoundingBox& query_bounds) const;
    
    /**
     * @brief Find nearest neighbors to a point
     * @param point Query point
     * @param k Number of neighbors to find
     * @return Vector of data indices sorted by distance
     */
    std::vector<size_t> nearestNeighbors(const Point2D& point, size_t k) const;
    
    /**
     * @brief Query objects within distance of a point
     * @param point Query point
     * @param distance Maximum distance
     * @return Vector of data indices within distance
     */
    std::vector<size_t> withinDistance(const Point2D& point, double distance) const;
    
    /**
     * @brief Clear all entries from the index
     */
    void clear();
    
    /**
     * @brief Get statistics about the index
     * @return String with index statistics
     */
    std::string getStats() const;
    
    /**
     * @brief Get total number of indexed objects
     */
    size_t size() const { return object_bounds_.size(); }

private:
    void insertHelper(RTreeNode* node, const BoundingBox& bounds, size_t data_index);
    void queryHelper(const RTreeNode* node, const BoundingBox& query_bounds, 
                    std::vector<size_t>& results) const;
    void splitNode(RTreeNode* node);
    BoundingBox calculateBounds(const RTreeNode* node) const;
    double calculateArea(const BoundingBox& bounds) const;
    double calculateEnlargement(const BoundingBox& existing, const BoundingBox& new_bounds) const;
    RTreeNode* chooseLeaf(const BoundingBox& bounds) const;
    void adjustTree(RTreeNode* node);
    void updateNodeBounds(RTreeNode* node);
    void updateLeafNodeBounds(RTreeNode* node);
    void updateInternalNodeBounds(RTreeNode* node);
    
    struct DistanceItem {
        size_t index;
        double distance;
        
        bool operator>(const DistanceItem& other) const {
            return distance > other.distance;
        }
    };
};

/**
 * @brief High-level spatial index interface
 * 
 * Provides a simple interface for spatial indexing of shapefile records
 */
class SpatialIndex {
private:
    RTree rtree_;
    std::vector<std::unique_ptr<ShapeRecord>>* records_;
    
public:
    SpatialIndex();
    ~SpatialIndex();
    
    /**
     * @brief Build index from shapefile records
     * @param records Vector of shape records to index
     */
    void buildIndex(std::vector<std::unique_ptr<ShapeRecord>>& records);
    
    /**
     * @brief Find records that intersect with bounding box
     * @param bounds Query bounding box
     * @return Vector of pointers to intersecting records
     */
    std::vector<ShapeRecord*> queryIntersects(const BoundingBox& bounds) const;
    
    /**
     * @brief Find records within distance of a point
     * @param point Query point
     * @param distance Maximum distance
     * @return Vector of pointers to records within distance
     */
    std::vector<ShapeRecord*> queryWithinDistance(const Point2D& point, double distance) const;
    
    /**
     * @brief Find nearest records to a point
     * @param point Query point
     * @param count Number of records to find
     * @return Vector of pointers to nearest records
     */
    std::vector<ShapeRecord*> queryNearest(const Point2D& point, size_t count) const;
    
    /**
     * @brief Check if a point is contained in any indexed polygon
     * @param point Query point
     * @return Pointer to containing polygon record, or nullptr
     */
    ShapeRecord* pointInPolygon(const Point2D& point) const;
    
    /**
     * @brief Get statistics about the spatial index
     */
    std::string getStats() const;
};

} // namespace gis

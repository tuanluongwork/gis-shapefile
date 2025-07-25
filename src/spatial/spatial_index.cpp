#include "gis/spatial_index.h"
#include <algorithm>
#include <queue>
#include <cmath>
#include <sstream>

namespace gis {

// R-Tree Implementation
RTree::RTree(size_t max_entries) 
    : max_entries_(max_entries)
    , min_entries_(max_entries / 2)
    , root_(std::make_unique<RTreeNode>(true)) {
}

RTree::~RTree() = default;

void RTree::insert(const BoundingBox& bounds, size_t data_index) {
    object_bounds_.push_back(bounds);
    insertHelper(root_.get(), bounds, data_index);
}

void RTree::insertHelper(RTreeNode* node, const BoundingBox& bounds, size_t data_index) {
    if (node->is_leaf) {
        // Add to leaf node
        node->data_indices.push_back(data_index);
        
        // Update node bounds
        if (node->data_indices.size() == 1) {
            node->bounds = bounds;
        } else {
            node->bounds.min_x = std::min(node->bounds.min_x, bounds.min_x);
            node->bounds.min_y = std::min(node->bounds.min_y, bounds.min_y);
            node->bounds.max_x = std::max(node->bounds.max_x, bounds.max_x);
            node->bounds.max_y = std::max(node->bounds.max_y, bounds.max_y);
        }
        
        // Check if node needs splitting
        if (node->data_indices.size() > max_entries_) {
            splitNode(node);
        }
    } else {
        // Find best child to insert into
        RTreeNode* best_child = nullptr;
        double min_enlargement = std::numeric_limits<double>::max();
        
        for (auto& child : node->children) {
            double enlargement = calculateEnlargement(child->bounds, bounds);
            if (enlargement < min_enlargement) {
                min_enlargement = enlargement;
                best_child = child.get();
            }
        }
        
        if (best_child) {
            insertHelper(best_child, bounds, data_index);
            
            // Update this node's bounds after insertion
            updateNodeBounds(node);
        }
    }
}

std::vector<size_t> RTree::query(const BoundingBox& query_bounds) const {
    std::vector<size_t> results;
    queryHelper(root_.get(), query_bounds, results);
    return results;
}

void RTree::queryHelper(const RTreeNode* node, const BoundingBox& query_bounds, 
                       std::vector<size_t>& results) const {
    if (!node->bounds.intersects(query_bounds)) {
        return;
    }
    
    if (node->is_leaf) {
        // Check each data item in leaf
        for (size_t idx : node->data_indices) {
            if (idx < object_bounds_.size() && object_bounds_[idx].intersects(query_bounds)) {
                results.push_back(idx);
            }
        }
    } else {
        // Recursively search children
        for (const auto& child : node->children) {
            queryHelper(child.get(), query_bounds, results);
        }
    }
}

std::vector<size_t> RTree::nearestNeighbors(const Point2D& point, size_t k) const {
    std::priority_queue<DistanceItem, std::vector<DistanceItem>, std::greater<DistanceItem>> pq;
    
    // For simplicity, this is a basic implementation
    // Production code would use a more sophisticated algorithm
    std::vector<DistanceItem> all_items;
    
    for (size_t i = 0; i < object_bounds_.size(); ++i) {
        const BoundingBox& bounds = object_bounds_[i];
        Point2D center((bounds.min_x + bounds.max_x) / 2.0, 
                      (bounds.min_y + bounds.max_y) / 2.0);
        
        double dx = point.x - center.x;
        double dy = point.y - center.y;
        double distance = std::sqrt(dx * dx + dy * dy);
        
        all_items.push_back({i, distance});
    }
    
    std::sort(all_items.begin(), all_items.end(), 
              [](const DistanceItem& a, const DistanceItem& b) {
                  return a.distance < b.distance;
              });
    
    std::vector<size_t> results;
    for (size_t i = 0; i < std::min(k, all_items.size()); ++i) {
        results.push_back(all_items[i].index);
    }
    
    return results;
}

std::vector<size_t> RTree::withinDistance(const Point2D& point, double distance) const {
    // Create bounding box around point
    BoundingBox query_bounds(point.x - distance, point.y - distance,
                            point.x + distance, point.y + distance);
    
    std::vector<size_t> candidates = query(query_bounds);
    std::vector<size_t> results;
    
    // Filter by exact distance
    for (size_t idx : candidates) {
        if (idx < object_bounds_.size()) {
            const BoundingBox& bounds = object_bounds_[idx];
            Point2D center((bounds.min_x + bounds.max_x) / 2.0, 
                          (bounds.min_y + bounds.max_y) / 2.0);
            
            double dx = point.x - center.x;
            double dy = point.y - center.y;
            double actual_distance = std::sqrt(dx * dx + dy * dy);
            
            if (actual_distance <= distance) {
                results.push_back(idx);
            }
        }
    }
    
    return results;
}

void RTree::splitNode(RTreeNode* node) {
    // Only split if node is overcrowded
    if ((!node->is_leaf && node->children.size() <= max_entries_) ||
        (node->is_leaf && node->data_indices.size() <= max_entries_)) {
        return;
    }
    
    // Create new sibling node
    auto new_node = std::make_unique<RTreeNode>(node->is_leaf);
    new_node->parent = node->parent;
    
    if (node->is_leaf) {
        // Split leaf node data
        size_t split_point = node->data_indices.size() / 2;
        
        new_node->data_indices.assign(node->data_indices.begin() + split_point, 
                                     node->data_indices.end());
        node->data_indices.resize(split_point);
        
        // Recalculate bounding boxes for both nodes
        updateLeafNodeBounds(node);
        updateLeafNodeBounds(new_node.get());
    } else {
        // Split internal node children
        size_t split_point = node->children.size() / 2;
        
        // Move children to new node
        for (size_t i = split_point; i < node->children.size(); ++i) {
            node->children[i]->parent = new_node.get();
            new_node->children.push_back(std::move(node->children[i]));
        }
        node->children.resize(split_point);
        
        // Recalculate bounding boxes for both nodes
        updateInternalNodeBounds(node);
        updateInternalNodeBounds(new_node.get());
    }
    
    // Handle parent integration
    if (node->parent == nullptr) {
        // Node is root - create new root
        auto new_root = std::make_unique<RTreeNode>(false);
        new_root->children.push_back(std::move(root_));
        new_root->children.push_back(std::move(new_node));
        
        // Set parent pointers
        new_root->children[0]->parent = new_root.get();
        new_root->children[1]->parent = new_root.get();
        
        // Update root bounds
        updateInternalNodeBounds(new_root.get());
        
        root_ = std::move(new_root);
    } else {
        // Add new node to parent
        RTreeNode* parent = node->parent;
        new_node->parent = parent;
        parent->children.push_back(std::move(new_node));
        
        // Update parent bounds
        updateInternalNodeBounds(parent);
        
        // Check if parent needs splitting
        if (parent->children.size() > max_entries_) {
            splitNode(parent);
        }
    }
}

double RTree::calculateEnlargement(const BoundingBox& existing, const BoundingBox& new_bounds) const {
    BoundingBox enlarged;
    enlarged.min_x = std::min(existing.min_x, new_bounds.min_x);
    enlarged.min_y = std::min(existing.min_y, new_bounds.min_y);
    enlarged.max_x = std::max(existing.max_x, new_bounds.max_x);
    enlarged.max_y = std::max(existing.max_y, new_bounds.max_y);
    
    return enlarged.area() - existing.area();
}

void RTree::updateNodeBounds(RTreeNode* node) {
    if (!node) return;
    
    if (node->is_leaf) {
        updateLeafNodeBounds(node);
    } else {
        updateInternalNodeBounds(node);
    }
}

void RTree::updateLeafNodeBounds(RTreeNode* node) {
    if (!node || !node->is_leaf || node->data_indices.empty()) return;
    
    bool first = true;
    for (size_t idx : node->data_indices) {
        if (idx < object_bounds_.size()) {
            const BoundingBox& bounds = object_bounds_[idx];
            if (first) {
                node->bounds = bounds;
                first = false;
            } else {
                node->bounds.min_x = std::min(node->bounds.min_x, bounds.min_x);
                node->bounds.min_y = std::min(node->bounds.min_y, bounds.min_y);
                node->bounds.max_x = std::max(node->bounds.max_x, bounds.max_x);
                node->bounds.max_y = std::max(node->bounds.max_y, bounds.max_y);
            }
        }
    }
}

void RTree::updateInternalNodeBounds(RTreeNode* node) {
    if (!node || node->is_leaf || node->children.empty()) return;
    
    bool first = true;
    for (const auto& child : node->children) {
        if (first) {
            node->bounds = child->bounds;
            first = false;
        } else {
            node->bounds.min_x = std::min(node->bounds.min_x, child->bounds.min_x);
            node->bounds.min_y = std::min(node->bounds.min_y, child->bounds.min_y);
            node->bounds.max_x = std::max(node->bounds.max_x, child->bounds.max_x);
            node->bounds.max_y = std::max(node->bounds.max_y, child->bounds.max_y);
        }
    }
}

void RTree::clear() {
    object_bounds_.clear();
    root_ = std::make_unique<RTreeNode>(true);
}

std::string RTree::getStats() const {
    std::ostringstream oss;
    oss << "R-Tree Statistics:\n";
    oss << "  Indexed Objects: " << object_bounds_.size() << "\n";
    oss << "  Max Entries per Node: " << max_entries_ << "\n";
    oss << "  Min Entries per Node: " << min_entries_ << "\n";
    return oss.str();
}

// SpatialIndex Implementation
SpatialIndex::SpatialIndex() : records_(nullptr) {}
SpatialIndex::~SpatialIndex() = default;

void SpatialIndex::buildIndex(std::vector<std::unique_ptr<ShapeRecord>>& records) {
    records_ = &records;
    rtree_.clear();
    
    for (size_t i = 0; i < records.size(); ++i) {
        const auto& record = records[i];
        if (record && record->geometry) {
            BoundingBox bounds = record->geometry->getBounds();
            rtree_.insert(bounds, i);
        }
    }
}

std::vector<ShapeRecord*> SpatialIndex::queryIntersects(const BoundingBox& bounds) const {
    std::vector<ShapeRecord*> results;
    
    if (!records_) return results;
    
    std::vector<size_t> indices = rtree_.query(bounds);
    
    for (size_t idx : indices) {
        if (idx < records_->size() && (*records_)[idx]) {
            results.push_back((*records_)[idx].get());
        }
    }
    
    return results;
}

std::vector<ShapeRecord*> SpatialIndex::queryWithinDistance(const Point2D& point, double distance) const {
    std::vector<ShapeRecord*> results;
    
    if (!records_) return results;
    
    std::vector<size_t> indices = rtree_.withinDistance(point, distance);
    
    for (size_t idx : indices) {
        if (idx < records_->size() && (*records_)[idx]) {
            results.push_back((*records_)[idx].get());
        }
    }
    
    return results;
}

std::vector<ShapeRecord*> SpatialIndex::queryNearest(const Point2D& point, size_t count) const {
    std::vector<ShapeRecord*> results;
    
    if (!records_) return results;
    
    std::vector<size_t> indices = rtree_.nearestNeighbors(point, count);
    
    for (size_t idx : indices) {
        if (idx < records_->size() && (*records_)[idx]) {
            results.push_back((*records_)[idx].get());
        }
    }
    
    return results;
}

ShapeRecord* SpatialIndex::pointInPolygon(const Point2D& point) const {
    if (!records_) return nullptr;
    
    // Create small bounding box around point for initial filtering
    BoundingBox point_bounds(point.x - 0.0001, point.y - 0.0001,
                            point.x + 0.0001, point.y + 0.0001);
    
    std::vector<size_t> candidates = rtree_.query(point_bounds);
    
    for (size_t idx : candidates) {
        if (idx < records_->size() && (*records_)[idx] && (*records_)[idx]->geometry) {
            //const auto& geometry = (*records_)[idx]->geometry;
            //
            //if (geometry->getType() == ShapeType::Polygon) {
            //    auto* polygon = dynamic_cast<const PolygonGeometry*>(geometry.get());
            //    if (polygon && polygon->contains(point)) {
            //        return (*records_)[idx].get();
            //    }
            //}
            return (*records_)[idx].get();
        }
    }
    
    return nullptr;
}

std::string SpatialIndex::getStats() const {
    std::ostringstream oss;
    oss << "Spatial Index Statistics:\n";
    oss << "  Indexed Records: " << (records_ ? records_->size() : 0) << "\n";
    oss << rtree_.getStats();
    return oss.str();
}

} // namespace gis

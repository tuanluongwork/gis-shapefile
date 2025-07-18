#include <gtest/gtest.h>
#include "gis/spatial_index.h"
#include "gis/geometry.h"
#include <vector>
#include <memory>

namespace gis {

class SpatialIndexTest : public ::testing::Test {
protected:
    void SetUp() override {
        spatialIndex = std::make_unique<SpatialIndex>();
    }

    void TearDown() override {
        spatialIndex.reset();
    }

    std::unique_ptr<SpatialIndex> spatialIndex;
};

TEST_F(SpatialIndexTest, DefaultConstruction) {
    EXPECT_NE(spatialIndex, nullptr);
}

TEST_F(SpatialIndexTest, InsertAndQuery) {
    // Insert some test geometries
    auto point1 = std::make_shared<Point>(10.0, 20.0);
    auto point2 = std::make_shared<Point>(30.0, 40.0);
    auto point3 = std::make_shared<Point>(50.0, 60.0);
    
    spatialIndex->insert(point1, BoundingBox(10.0, 20.0, 10.0, 20.0));
    spatialIndex->insert(point2, BoundingBox(30.0, 40.0, 30.0, 40.0));
    spatialIndex->insert(point3, BoundingBox(50.0, 60.0, 50.0, 60.0));
    
    // Query a region that should contain point1 and point2
    BoundingBox queryRegion(0.0, 0.0, 35.0, 45.0);
    auto results = spatialIndex->query(queryRegion);
    
    EXPECT_GE(results.size(), 2);
}

TEST_F(SpatialIndexTest, EmptyIndexQuery) {
    BoundingBox queryRegion(0.0, 0.0, 100.0, 100.0);
    auto results = spatialIndex->query(queryRegion);
    
    EXPECT_TRUE(results.empty());
}

TEST_F(SpatialIndexTest, BoundingBoxIntersection) {
    BoundingBox box1(0.0, 0.0, 10.0, 10.0);
    BoundingBox box2(5.0, 5.0, 15.0, 15.0);
    BoundingBox box3(20.0, 20.0, 30.0, 30.0);
    
    EXPECT_TRUE(box1.intersects(box2));
    EXPECT_TRUE(box2.intersects(box1));
    EXPECT_FALSE(box1.intersects(box3));
    EXPECT_FALSE(box3.intersects(box1));
}

TEST_F(SpatialIndexTest, NearestNeighborSearch) {
    // Insert test points
    auto point1 = std::make_shared<Point>(0.0, 0.0);
    auto point2 = std::make_shared<Point>(10.0, 10.0);
    auto point3 = std::make_shared<Point>(20.0, 20.0);
    
    spatialIndex->insert(point1, BoundingBox(0.0, 0.0, 0.0, 0.0));
    spatialIndex->insert(point2, BoundingBox(10.0, 10.0, 10.0, 10.0));
    spatialIndex->insert(point3, BoundingBox(20.0, 20.0, 20.0, 20.0));
    
    // Find nearest to (5, 5) - should be point1
    Point queryPoint(5.0, 5.0);
    auto nearest = spatialIndex->nearestNeighbor(queryPoint);
    
    EXPECT_NE(nearest, nullptr);
    // The nearest point should be point1 (0,0) which is closest to (5,5)
}

TEST_F(SpatialIndexTest, KNearestNeighbors) {
    // Insert multiple test points
    std::vector<std::shared_ptr<Point>> points = {
        std::make_shared<Point>(0.0, 0.0),
        std::make_shared<Point>(1.0, 1.0),
        std::make_shared<Point>(2.0, 2.0),
        std::make_shared<Point>(10.0, 10.0),
        std::make_shared<Point>(20.0, 20.0)
    };
    
    for (auto& point : points) {
        BoundingBox bbox(point->x(), point->y(), point->x(), point->y());
        spatialIndex->insert(point, bbox);
    }
    
    Point queryPoint(0.5, 0.5);
    auto nearest = spatialIndex->kNearestNeighbors(queryPoint, 3);
    
    EXPECT_EQ(nearest.size(), 3);
    // The three nearest should be the first three points
}

TEST_F(SpatialIndexTest, LargeDatasetPerformance) {
    const int numPoints = 1000;
    
    // Insert many points
    for (int i = 0; i < numPoints; ++i) {
        auto point = std::make_shared<Point>(i * 0.1, i * 0.1);
        BoundingBox bbox(point->x(), point->y(), point->x(), point->y());
        spatialIndex->insert(point, bbox);
    }
    
    // Query should still be fast
    BoundingBox queryRegion(0.0, 0.0, 50.0, 50.0);
    auto results = spatialIndex->query(queryRegion);
    
    EXPECT_GT(results.size(), 0);
    EXPECT_LE(results.size(), numPoints);
}

} // namespace gis

#include <gtest/gtest.h>
#include "gis/shapefile_reader.h"
#include "gis/geometry.h"
#include <memory>

namespace gis {

class ShapefileReaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test setup
    }

    void TearDown() override {
        // Test cleanup
    }
};

TEST_F(ShapefileReaderTest, DefaultConstruction) {
    auto reader = std::make_unique<ShapefileReader>();
    EXPECT_NE(reader, nullptr);
}

TEST_F(ShapefileReaderTest, InvalidFilePath) {
    ShapefileReader reader;
    EXPECT_FALSE(reader.open("nonexistent_file.shp"));
}

TEST_F(ShapefileReaderTest, GeometryCreation) {
    Point point(100.0, 200.0);
    EXPECT_DOUBLE_EQ(point.x(), 100.0);
    EXPECT_DOUBLE_EQ(point.y(), 200.0);
}

TEST_F(ShapefileReaderTest, BoundingBoxCalculation) {
    BoundingBox bbox(0.0, 0.0, 100.0, 100.0);
    Point point1(50.0, 50.0);
    Point point2(150.0, 50.0);
    
    EXPECT_TRUE(bbox.contains(point1));
    EXPECT_FALSE(bbox.contains(point2));
}

TEST_F(ShapefileReaderTest, PolygonPointInside) {
    // Create a simple square polygon
    std::vector<Point> vertices = {
        Point(0.0, 0.0),
        Point(10.0, 0.0),
        Point(10.0, 10.0),
        Point(0.0, 10.0),
        Point(0.0, 0.0)  // Close the polygon
    };
    
    Polygon polygon(vertices);
    
    Point inside(5.0, 5.0);
    Point outside(15.0, 15.0);
    
    EXPECT_TRUE(polygon.contains(inside));
    EXPECT_FALSE(polygon.contains(outside));
}

} // namespace gis

#include <gtest/gtest.h>
#include "gis/geocoder.h"
#include "gis/geometry.h"

namespace gis {

class GeocoderTest : public ::testing::Test {
protected:
    void SetUp() override {
        geocoder = std::make_unique<Geocoder>();
    }

    void TearDown() override {
        geocoder.reset();
    }

    std::unique_ptr<Geocoder> geocoder;
};

TEST_F(GeocoderTest, DefaultConstruction) {
    EXPECT_NE(geocoder, nullptr);
}

TEST_F(GeocoderTest, AddressFormatting) {
    Address addr;
    addr.streetNumber = "123";
    addr.streetName = "Main Street";
    addr.city = "Anytown";
    addr.state = "CA";
    addr.zipCode = "90210";
    
    std::string formatted = geocoder->formatAddress(addr);
    EXPECT_FALSE(formatted.empty());
    EXPECT_TRUE(formatted.find("123") != std::string::npos);
    EXPECT_TRUE(formatted.find("Main Street") != std::string::npos);
}

TEST_F(GeocoderTest, CoordinateValidation) {
    Point validPoint(40.7128, -74.0060);  // New York City
    Point invalidLatitude(91.0, -74.0060);  // Invalid latitude > 90
    Point invalidLongitude(40.7128, 181.0); // Invalid longitude > 180
    
    EXPECT_TRUE(geocoder->isValidCoordinate(validPoint));
    EXPECT_FALSE(geocoder->isValidCoordinate(invalidLatitude));
    EXPECT_FALSE(geocoder->isValidCoordinate(invalidLongitude));
}

TEST_F(GeocoderTest, DistanceCalculation) {
    Point p1(40.7128, -74.0060);  // New York City
    Point p2(34.0522, -118.2437); // Los Angeles
    
    double distance = geocoder->calculateDistance(p1, p2);
    
    // Distance between NYC and LA is approximately 3944 km
    EXPECT_GT(distance, 3900.0);
    EXPECT_LT(distance, 4000.0);
}

TEST_F(GeocoderTest, EmptyAddressHandling) {
    Address emptyAddr;
    
    auto result = geocoder->geocode(emptyAddr);
    EXPECT_FALSE(result.has_value());
}

TEST_F(GeocoderTest, ParseAddressString) {
    std::string addressStr = "123 Main St, Anytown, CA 90210";
    
    auto parsed = geocoder->parseAddress(addressStr);
    EXPECT_TRUE(parsed.has_value());
    
    if (parsed) {
        EXPECT_EQ(parsed->streetNumber, "123");
        EXPECT_TRUE(parsed->streetName.find("Main") != std::string::npos);
        EXPECT_EQ(parsed->state, "CA");
        EXPECT_EQ(parsed->zipCode, "90210");
    }
}

} // namespace gis

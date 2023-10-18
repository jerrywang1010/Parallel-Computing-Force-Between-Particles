#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include "main.cpp"

TEST(SetupPointChargesTest, TestFileNotFound) {
    std::vector<point_charge> charges = setup_point_charges("nonexistent-file.csv");
    EXPECT_TRUE(charges.empty());
}

TEST(SetupPointChargesTest, TestFileOpen) {
    std::vector<point_charge> charges = setup_point_charges("particles-student-1.csv");
    EXPECT_FALSE(charges.empty());
}

TEST(SetupPointChargesTest, TestFileContent) {
    std::vector<point_charge> charges = setup_point_charges("particles-student-1.csv");
    std::ifstream file("particles-student-1.csv");
    std::string line;
    int line_count = 0;
    while (getline(file, line)) {
        std::stringstream ss(line);
        point_charge charge;
        char comma, polarity;
        ss >> charge.x >> comma >> charge.y >> comma >> polarity;
        EXPECT_EQ(charges[line_count].x, charge.x);
        EXPECT_EQ(charges[line_count].y, charge.y);
        line_count++;
    }
}
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include <fstream>
#include <sstream>

constexpr double kq1q2 = 8.99e9 * 1.6e-19 * 1.6e-19;

struct point_charge {
    int idx;
    int x;
    int y;
    int nearest_neighbor_idx;
};

inline double distance_between_square(const point_charge & p1, const point_charge & p2) {
    return (pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2)) * 1e-20;
}

void print_force(const std::vector<double> forces) {
    for (int i = 0; i < forces.size(); i ++) {
        std::cout << "ID: " << i + 1 << ", Force=" << forces[i] << std::endl;
    }
}

std::vector<point_charge> setup_point_charges(const std::string& filename, int max_line=1000) {
    std::vector<point_charge> charges;
    std::ifstream file(filename);
    if (!file) {
        return charges;
    }

    std::string line;
    int line_count = 0;
    while (getline(file, line) && line_count < max_line) {
        std::stringstream ss(line);
        point_charge charge;
        char comma, polarity;
        charge.idx = line_count;
        ss >> charge.x >> comma >> charge.y >> comma >> polarity;
        charges.push_back(charge);
        line_count ++;
    }

    file.close();
    
    if (charges.size() < 2) {
        std::cerr << "too few points!" << std::endl;
        return charges;
    }

    charges[0].nearest_neighbor_idx = 1;
    charges.back().nearest_neighbor_idx = charges.size() - 2;
    for (int i = 1; i < charges.size() - 1; i ++) {
        double prev_distance = distance_between_square(charges[i], charges[i - 1]);
        double next_distance = distance_between_square(charges[i], charges[i + 1]);
        if (prev_distance < next_distance) {
            charges[i].nearest_neighbor_idx = i - 1;
        } else {
            charges[i].nearest_neighbor_idx = i + 1;
        }
    }

    int old_size = charges.size();
    while (charges.size() < max_line) {
        for (int i = 0; i < old_size; i ++) {
            charges.push_back(charges[i]);
            charges.back().idx = charges.size() - 1;
            if (charges.size() == max_line) break;
        }
    }
    return charges;
}
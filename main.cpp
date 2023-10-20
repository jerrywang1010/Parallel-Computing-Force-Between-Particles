#include <chrono>
#include <thread>
#include <mutex>
#include "common.h"


void test_file_not_found() {
    std::vector<point_charge> charges = setup_point_charges("./nonexistent-file.csv");
    assert(charges.empty());
}

void test_file_open() {
    std::vector<point_charge> charges = setup_point_charges("./particles-student-1.csv", 10);
    assert(!charges.empty());
}

void test_file_content() {
    std::vector<point_charge> charges = setup_point_charges("./particles-student-1.csv", 1000);
    std::ifstream file("./particles-student-1.csv");
    std::string line;
    int line_count = 0;
    while (getline(file, line) && line_count < charges.size()) {
        std::stringstream ss(line);
        point_charge charge;
        char comma, polarity;
        ss >> charge.x >> comma >> charge.y >> comma >> polarity;
        assert(charges[line_count].x == charge.x);
        assert(charges[line_count].y == charge.y);
        line_count++;
    }
}

std::vector<double> serial_calculation(const std::vector<point_charge>& charges) {
    std::vector<double> ans;
    for (int i = 0; i < charges.size(); i ++) {
        ans.push_back(kq1q2 / distance_between_square(charges[i], charges[charges[i].nearest_neighbor_idx]));
    }
    return ans;
}


void thread_worker(const std::vector<point_charge> charges, int start, int end, std::vector<double>& ans) {
    for (int i = start; i < end; i ++) {
        ans[i] = kq1q2 / distance_between_square(charges[i], charges[charges[i].nearest_neighbor_idx]);
    }
}


std::vector<double> multithread_calculation(const std::vector<point_charge>& charges, int num_threads=4) {
    std::vector<double> ans(charges.size());
    std::vector<std::thread> threads;

    int chunk_size = charges.size() / num_threads;
    int remainder  = charges.size() % num_threads;

    if (chunk_size < 1) {
        std::cerr << "too many threads, not enough data!" << " num_threads=" << num_threads << ", num_particles=" << charges.size() << std::endl;
        return ans;
    }

    int start = 0;
    for(int i = 0; i < num_threads; i ++) {
        int size = (i < remainder) ? chunk_size + 1 : chunk_size;
        threads.push_back(std::thread(thread_worker, std::cref(charges), start, start + size, std::ref(ans)));
        start += size;
    }

    for (auto & thread : threads) {
        thread.join();
    }
    return ans;
}


int main(int argc, char* argv[]) {
    if (argc < 3 || std::string(argv[1]).find("mode=") == std::string::npos || std::string(argv[2]).find("num_particles=") == std::string::npos) {
        std::cerr << "Usage: ./ForceCalculation mode={1-2} num_particles={d+} num_threads={d+}" << std::endl;
        return -1;
    }
    test_file_not_found();
    test_file_open();
    test_file_content();

    int mode = std::stoi(std::string(argv[1]).substr(5));
    int num_particles = std::stoi(std::string(argv[2]).substr(14));
    int num_threads = 0;

    if (mode == 1) {
        if (argc !=3) {
        std::cerr << "Usage: ./ForceCalculation mode={1-2} num_particles={d+}" << std::endl;
        return -1;
    }
    } else if (mode == 2) {
        if (argc != 4) {
            std::cerr << "Usage: ./ForceCalculation mode={1-2} num_particles={d+} num_threads={d+}" << std::endl;
            return -1;
        }
        num_threads = std::stoi(std::string(argv[3]).substr(12));
    }

    std::vector<point_charge> charges = setup_point_charges("./particles-student-1.csv", num_particles);
    std::vector<double> ans;

    auto start = std::chrono::high_resolution_clock::now();
    switch (mode)
    {
        case 1:
            ans = serial_calculation(charges);
            break;

        case 2:
            ans = multithread_calculation(charges, num_threads);
            break;
        
        default:
            std::cerr << "Invalid mode value: " << mode << std::endl;
            return -1;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "Function for mode=" << mode << " took " << duration << " microseconds." << std::endl;

    print_force(ans);
    
    return 0;
}
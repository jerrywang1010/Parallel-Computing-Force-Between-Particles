#include <chrono>
#include <thread>
#include <mutex>
#include "common.h"


void testFileNotFound() {
    std::vector<point_charge> charges = setup_point_charges("../nonexistent-file.csv");
    assert(charges.empty());
}

void testFileOpen() {
    std::vector<point_charge> charges = setup_point_charges("../particles-student-1.csv");
    assert(!charges.empty());
}

void testFileContent() {
    std::vector<point_charge> charges = setup_point_charges("../particles-student-1.csv");
    std::ifstream file("..//particles-student-1.csv");
    std::string line;
    int line_count = 0;
    while (getline(file, line)) {
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
        ans.push_back(kq1q2 / distance_between(charges[i], charges[charges[i].nearest_neighbor_idx]));
    }
    return ans;
}


void thread_worker(const std::vector<point_charge> charges, int start, int end, std::vector<double>& ans) {
    for (int i = start; i < end; i ++) {
        ans[i] = kq1q2 / distance_between(charges[i], charges[charges[i].nearest_neighbor_idx]);
    }
}


std::vector<double> multithread_calculation(const std::vector<point_charge>& charges, int num_threads=4) {
    std::vector<double> ans(charges.size());
    std::vector<std::thread> threads;

    int chunk_size = charges.size() / num_threads;
    if (chunk_size < 1) {
        std::cerr << "too many threads, not enough data!" << std::endl;
        return ans;
    }

    for(int i = 0; i < num_threads; i ++) {
        int start = i * chunk_size;
        int end = i == num_threads - 1 ? charges.size() : start + chunk_size;
        threads.push_back(std::thread(thread_worker, std::cref(charges), start, end, std::ref(ans)));
    }

    for (auto & thread : threads) {
        thread.join();
    }
    return ans;
}


int main(int argc, char* argv[]) {
    if (argc < 3 || std::string(argv[1]).find("mode=") == std::string::npos || std::string(argv[2]).find("num_particles=") == std::string::npos) {
        std::cerr << "Usage: ./ForceCalculation mode={1-3} num_particles={d+}" << std::endl;
        return -1;
    }
    testFileNotFound();
    testFileOpen();
    testFileContent();

    int mode = std::stoi(std::string(argv[1]).substr(5));
    int num_particles = std::stoi(std::string(argv[2]).substr(14));
    std::vector<point_charge> charges = setup_point_charges("../particles-student-1.csv", num_particles);
    std::vector<double> ans;

    auto start = std::chrono::high_resolution_clock::now();
    switch (mode)
    {
        case 1:
            ans = serial_calculation(charges);
            for (int i = 0; i < ans.size(); i ++) {
                std::cout << "ID: " << i << ", Force=" << ans[i] << std::endl;
            }
            break;

        case 2:
            ans = multithread_calculation(charges);
            print_force(ans);
            break;
        
        default:
            std::cerr << "Invalid mode value: " << mode << std::endl;
            return -1;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "Function for mode=" << mode << " took " << duration << " microseconds." << std::endl;
    
    return 0;
}
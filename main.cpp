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


// large time between the earliest thread stated that the main thread finished creating threads, i.e. started to wait for join

void thread_worker(const std::vector<point_charge> charges, int start, int end, std::vector<double>& ans) {
    std::thread::id thread_id = std::this_thread::get_id();
    auto start_time = std::chrono::high_resolution_clock::now();
    auto start_now = std::chrono::system_clock::now();
    for (int i = start; i < end; i ++) {
        ans[i] = kq1q2 / distance_between_square(charges[i], charges[charges[i].nearest_neighbor_idx]);
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    auto now = std::chrono::system_clock::now();
    // std::cout << "Thread: " << thread_id << " finished in " << duration.count() 
    //         << " microseconds, started at:" << std::chrono::duration_cast<std::chrono::microseconds>(start_now.time_since_epoch()).count()
    //         << ", finished at:" << std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count() << "\n";
}


std::vector<double> multithread_calculation(const std::vector<point_charge>& charges, int num_threads=4) {
    std::vector<double> ans(charges.size());
    std::vector<std::thread> threads(num_threads);

    int chunk_size = charges.size() / num_threads;
    int remainder  = charges.size() % num_threads;

    if (chunk_size < 1) {
        std::cerr << "too many threads, not enough data!" << " num_threads=" << num_threads << ", num_particles=" << charges.size() << std::endl;
        return ans;
    }

    int start = 0;
    for(int i = 0; i < num_threads; i ++) {
        int size = (i < remainder) ? chunk_size + 1 : chunk_size;
        // threads.push_back(std::thread(thread_worker, std::cref(charges), start, start + size, std::ref(ans)));
        threads[i] = (std::thread(thread_worker, std::cref(charges), start, start + size, std::ref(ans)));
        start += size;
    }


    // for (auto & thread : threads) {
    //     thread.join();
    // }
    // auto join_start_time = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < threads.size(); i ++) {
        // auto join_start_time_i = std::chrono::system_clock::now();
        // auto join_start = std::chrono::high_resolution_clock::now();
        // std::thread::id tid = threads[i].get_id();
        threads[i].join();
        // auto join_end = std::chrono::high_resolution_clock::now();
        // auto join_duration = std::chrono::duration_cast<std::chrono::microseconds>(join_end - join_start).count();
        // std::cout << "Time to join thread id:" << tid << " : " << join_duration << " microseconds. started at: " << std::chrono::duration_cast<std::chrono::microseconds>(join_start_time_i.time_since_epoch()).count() << std::endl;
    }
    // auto join_end_time = std::chrono::high_resolution_clock::now();
    // std::cout << "Time to join thread: " << std::chrono::duration_cast<std::chrono::microseconds>(join_end_time - join_start_time).count() << " microseconds." << std::endl;
    return ans;
}


int main(int argc, char* argv[]) {
    if (argc < 3 || std::string(argv[1]).find("mode=") == std::string::npos || std::string(argv[2]).find("num_particles=") == std::string::npos) {
        std::cerr << "Usage: ./ForceCalculation mode={1-2} num_particles={d+} num_threads={d+}" << std::endl;
        return -1;
    }

    auto start = std::chrono::high_resolution_clock::now();
    test_file_not_found();
    test_file_open();
    test_file_content();
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Time to run tests: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " microseconds." << std::endl;

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

    start = std::chrono::high_resolution_clock::now();
    std::vector<point_charge> charges = setup_point_charges("./particles-student-1.csv", num_particles);
    end = std::chrono::high_resolution_clock::now();
    std::cout << "Time to read file: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " microseconds." << std::endl;

    std::vector<double> ans;

    start = std::chrono::high_resolution_clock::now();
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
    end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "Time to calculate force for mode=" << mode << " took " << duration << " microseconds." << std::endl;
    // print_force(ans);
    return 0;
}
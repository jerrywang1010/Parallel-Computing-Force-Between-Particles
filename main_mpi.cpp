#include <cstddef>
#include <queue>
#include <mutex>
#include <mpi.h>
#include <future>
#include "common.h"

MPI_Datatype MPI_POINT_CHARGE;
std::mutex mtx;

std::vector<double> thread_worker(std::queue<std::vector<point_charge>>& charges_queue) {
    std::vector<double> results;
    while (true) {
        std::vector<point_charge> charges;
        {
            std::lock_guard<std::mutex> lk(mtx);
            if (charges_queue.empty()) return results;
            charges = std::move(charges_queue.front());
            charges_queue.pop();
        }
        for (const auto & charge : charges) {
            results.push_back(kq1q2 / distance_between(charge, charges[charge.nearest_neighbor_idx]));
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: mpirun -np {num_proc} ./ForceCalculationMPI {num_threads} {num_particles}" << std::endl;
        return -1;
    }
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int blockLengths[3] = {1, 1, 1};
    MPI_Aint offsets[3] = {
        offsetof(point_charge, x),
        offsetof(point_charge, y),
        offsetof(point_charge, nearest_neighbor_idx)
    };

    MPI_Datatype types[3] = {MPI_INT, MPI_INT, MPI_INT};
    MPI_Type_create_struct(3, blockLengths, offsets, types, &MPI_POINT_CHARGE);
    MPI_Type_commit(&MPI_POINT_CHARGE);

    const int num_threads = std::stoi(argv[1]);
    const int num_particles = std::stoi(argv[2]);

    std::vector<point_charge> data;
    int data_size = 0;
    if (rank == 0) {
        data = std::move(setup_point_charges("../particles-student-1.csv", num_particles));
        data_size = data.size();
    }

    // Broadcast the size of the input to all processes
    MPI_Bcast(&data_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    int remainder = data_size % size;
    int chunk_size = data_size / size;
    int adjusted_chunk_size = (rank < remainder) ? chunk_size + 1 : chunk_size;

    std::vector<point_charge> local_data(adjusted_chunk_size);

    MPI_Scatter(&data[0], adjusted_chunk_size, MPI_POINT_CHARGE, &local_data[0], adjusted_chunk_size, MPI_POINT_CHARGE, 0, MPI_COMM_WORLD);

    std::queue<std::vector<point_charge>> worker_queue;
    int thread_chunk_size = adjusted_chunk_size / num_threads;
    for (int i = 0; i < num_threads; i ++) {
        int start = i * thread_chunk_size;
        int end = (i == num_threads - 1) ? adjusted_chunk_size : start + thread_chunk_size;
        std::cout << "Rank=" << rank << ", thread #" << i << ", start=" << start << ", end=" << end << ", start.x=" 
                    << local_data[start].x << ", start.y=" << local_data[start].y << std::endl;
        worker_queue.push(std::vector<point_charge>(local_data.begin() + start, local_data.begin() + end));
    }

    std::vector<std::future<std::vector<double>>> futures;
    for (int i = 0; i < num_threads; i ++) {
        futures.push_back(std::async(std::launch::async, thread_worker, std::ref(worker_queue)));
    }

    std::vector<double> local_result;
    for (auto & f : futures) {
        std::vector<double> thread_result = f.get();
        local_result.insert(local_result.end(), thread_result.begin(), thread_result.end());
    }

    std::vector<double> final_results(data.size());
    MPI_Gather(&local_result[0], adjusted_chunk_size, MPI_DOUBLE, &final_results[0], adjusted_chunk_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        // print_force(final_results);
    }

    MPI_Type_free(&MPI_POINT_CHARGE);
    MPI_Finalize();
    return 0;
}
#include <cstddef>
#include <queue>
#include <mutex>
#include <mpi.h>
#include <future>
#include "common.h"

MPI_Datatype MPI_POINT_CHARGE;
std::mutex mtx;


void thread_worker(std::queue<std::vector<point_charge>>& charges_queue, const std::vector<point_charge>& all_charges, std::vector<double>& results, int offset) {
    while (true) {
        std::vector<point_charge> charges;
        {
            std::lock_guard<std::mutex> lk(mtx);
            if (charges_queue.empty()) {
                return;
            }
            charges = std::move(charges_queue.front());
            charges_queue.pop();
        }
        for (const auto & charge : charges) {
            results[charge.idx - offset] = (kq1q2 / distance_between_square(charge, all_charges[charge.nearest_neighbor_idx]));
            // std::cout << "calculating force between x1=" << charge.x << ", y1=" << charge.y 
            //           << ", x2=" << all_charges[charge.nearest_neighbor_idx].x << ", y2=" << all_charges[charge.nearest_neighbor_idx].y
            //           << ", force=" << kq1q2 / distance_between_square(charge, all_charges[charge.nearest_neighbor_idx]) << std::endl;
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: mpirun -np {num_proc} ./build/ForceCalculationMPI {num_threads} {num_particles}" << std::endl;
        return -1;
    }
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int blockLengths[4] = {1, 1, 1, 1};
    MPI_Aint offsets[4] = {
        offsetof(point_charge, idx),
        offsetof(point_charge, x),
        offsetof(point_charge, y),
        offsetof(point_charge, nearest_neighbor_idx)
    };

    MPI_Datatype types[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};
    MPI_Type_create_struct(4, blockLengths, offsets, types, &MPI_POINT_CHARGE);
    MPI_Type_commit(&MPI_POINT_CHARGE);

    const int num_threads = std::stoi(argv[1]);
    const int num_particles = std::stoi(argv[2]);

    std::vector<point_charge> all_point_charges;
    int data_size = 0;
    if (rank == 0) {
        double start_time = MPI_Wtime();
        all_point_charges = std::move(setup_point_charges("./particles-student-1.csv", num_particles));
        double end_time = MPI_Wtime();
        double total_time = end_time - start_time;
        std::cout << "Time to read file: " << total_time * 1E6 << " microseconds." << std::endl;
        data_size = all_point_charges.size();
    }


    double start_time = 0;
    if (rank == 0) {
        start_time = MPI_Wtime();
    }
    // Broadcast the size of the input to all processes
    MPI_Bcast(&data_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Resize the vector for all other processes
    if (rank != 0) {
        all_point_charges.resize(data_size);
    }

    // Broadcast all point charges for read, so they don't need file IO
    MPI_Bcast(&all_point_charges[0], data_size, MPI_POINT_CHARGE, 0, MPI_COMM_WORLD);

    std::vector<int> sendcounts(size);
    std::vector<int> displs(size);
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sendcounts[i] = (i < data_size % size) ? data_size / size + 1 : data_size / size;
        displs[i] = sum;
        sum += sendcounts[i];
    }

    int adjusted_chunk_size = sendcounts[rank];
    std::vector<point_charge> local_data(adjusted_chunk_size);

    MPI_Scatterv(&all_point_charges[0], sendcounts.data(), displs.data(), MPI_POINT_CHARGE, &local_data[0], sendcounts[rank], MPI_POINT_CHARGE, 0, MPI_COMM_WORLD);

    std::vector<double> local_result(adjusted_chunk_size, -1);
    std::queue<std::vector<point_charge>> worker_queue;
    int thread_chunk_size = adjusted_chunk_size / num_threads;
    for (int i = 0; i < num_threads; i ++) {
        int start = i * thread_chunk_size;
        int end = (i == num_threads - 1) ? adjusted_chunk_size : start + thread_chunk_size;
        // std::cout << "Rank=" << rank << ", thread #" << i << ", start=" << start << ", end=" << end << ", start_idx=" << (local_data.begin() + start)->idx << ", end_idx=" << (local_data.begin() + end)->idx << std::endl;
        worker_queue.push(std::vector<point_charge>(local_data.begin() + start, local_data.begin() + end));
    }

    std::vector<std::future<void>> futures;
    for (int i = 0; i < num_threads; i ++) {
        futures.push_back(std::async(std::launch::async, thread_worker, std::ref(worker_queue), std::cref(all_point_charges), std::ref(local_result), local_data.begin()->idx));
    }

    for (auto & f : futures) {
        f.get();
    }

    std::vector<double> final_results;
    if (rank == 0) {
        final_results.resize(all_point_charges.size());
    }
    // MPI_Gather(&local_result[0], adjusted_chunk_size, MPI_DOUBLE, &final_results[0], adjusted_chunk_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    // MPI_Gatherv(&local_result[0], adjusted_chunk_size, MPI_DOUBLE, &final_results[0], sendcounts.data(), displs.data(), MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Gatherv(local_result.data(), local_result.size(), MPI_DOUBLE, final_results.data(), sendcounts.data(), displs.data(), MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        // print_force(final_results);
        double end_time = MPI_Wtime();
        double total_time = end_time - start_time;
        std::cout << "Time to calculate force: " << total_time * 1E6 << " microseconds." << std::endl;
    }

    MPI_Type_free(&MPI_POINT_CHARGE);
    MPI_Finalize();
    return 0;
}
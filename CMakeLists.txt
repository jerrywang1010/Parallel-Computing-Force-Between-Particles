# CMakeLists.txt
cmake_minimum_required(VERSION 3.0)

project(ForceCalculation)

# Add source files
add_executable(ForceCalculation main.cpp)

# Find MPI
find_package(MPI REQUIRED)
# Include MPI directories
include_directories(${MPI_INCLUDE_PATH})

add_executable(ForceCalculationMPI main_mpi.cpp)
# Link against MPI libraries
target_link_libraries(ForceCalculationMPI ${MPI_LIBRARIES})

import subprocess
import sys

force_calculation_path = './build/ForceCalculation'
force_calculation_mpi_path = './build/ForceCalculationMPI'

def main():
    print('please enter the mode(1-3): ')
    mode = int(input())
    print('please enter number of particles: ')
    num_particles = int(input())
    if mode == 1:
        print(f"calculating forces serially for {num_particles} points")
        subprocess.run([force_calculation_path, 'mode=1', f'num_particles={num_particles}'])
    elif mode == 2:
        print('please enter number of threads: ')
        num_threads = int(input())
        subprocess.run([force_calculation_path, 'mode=2', f'num_particles={num_particles}', f'num_threads={num_threads}'])
    elif mode == 3:
        print('please enter number of processes: ')
        num_proc = int(input())
        print('please enter number of threads: ')
        num_threads = int(input())
        subprocess.run(['mpirun', '-np', str(num_proc), force_calculation_mpi_path, str(num_threads), str(num_particles)])
    else:
        print("Invalid mode")

if __name__ == '__main__':
    main()
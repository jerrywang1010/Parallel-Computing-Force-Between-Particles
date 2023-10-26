# Parallel-Computing-Force-Between-Particles

# steps to compile the program

```
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
cd ..
```

# steps to run the program

```
in lab1/
python3 wrapper.py and follow prompts
```

# Mode 1 example

```
jerry_wang@jerry-desktop:~/ece1747/lab1$ python3 wrapper.py
please enter the mode(1-3):
1
please enter number of particles:
1000
calculating forces serially for 1000 points
Time to run tests: 918 microseconds.
Time to read file: 460 microseconds.
Time to calculate force for mode=1 took 95 microseconds.
```

# Mode 2 example

```
jerry_wang@jerry-desktop:~/ece1747/lab1$ python3 wrapper.py
please enter the mode(1-3):
2
please enter number of particles:
100000
please enter number of threads:
8
Time to run tests: 824 microseconds.
Time to read file: 45083 microseconds.
Time to calculate force for mode=2 took 4683 microseconds.
```

# Mode 3 example

```
jerry_wang@jerry-desktop:~/ece1747/lab1$ python3 wrapper.py
please enter the mode(1-3):
3
please enter number of particles:
100000
please enter number of processes:
4
please enter number of threads:
4
Time to read file: 47343.9 microseconds.
Time to calculate force: 3889.03 microseconds.
```

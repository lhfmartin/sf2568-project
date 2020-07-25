# SF2568 Project: Parallel Bitonic Merge Sort

Implementation of bitonic merge sort to sort randomly generated numbers.  
Written in C with MPI.

## To use the program

Compilation

`make`

Run the program using P nodes to sort N numbers
1. `mpirun -np P ./a.out N`
2. Select mode. If mode 0 is chosen, the randomly generated numbers will be written in before.txt and the sorted result will be stored in after.txt. If mode 1 is chosen, the time used for executing the sorting algorithm will be written in t.txt.

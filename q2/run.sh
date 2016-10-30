#!/bin/bash

set -e

make clean

find . -type f -name '*.csv' -delete

make

num_proc=1
max=48
while [ "$num_proc" -lt "$max" ] 
do
    mpirun -n $num_proc ./main 0 >> "stats_mpi_barrier.csv"
    num_proc=$(($num_proc+1))
done

num_proc=1
max=48
while [ "$num_proc" -lt "$max" ] 
do
    mpirun -n $num_proc ./main 1 >> "stats_my_barrier.csv"
    num_proc=$(($num_proc+1))
done
:

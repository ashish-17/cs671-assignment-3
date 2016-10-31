#!/bin/bash

#SBATCH -J MPI_BARRIER
#SBATCH -o MPI_BARRIER.%J.stdout
#SBATCH -e MPI_BARRIER.%J.stderr
#SBATCH -p main
#SBATCH --reservation mp002
#SBATCH -N 2
#SBATCH -t 00:10:00

#Uncomment following lines before running on caliburn
#cd $HOME/q2
#module load openmpi
#sleep 3

set -e

make clean

find . -type f -name '*.csv' -delete

make

num_proc=1
max=65
while [ "$num_proc" -lt "$max" ] 
do
    mpirun -n $num_proc ./main 0 1000 >> "stats_mpi_barrier.csv"
    num_proc=$(($num_proc+1))
done

num_proc=1
max=65
while [ "$num_proc" -lt "$max" ] 
do
    mpirun -n $num_proc ./main 1 1000 >> "stats_my_barrier.csv"
    num_proc=$(($num_proc+1))
done
:

#!/bin/bash

#SBATCH -J MPI_MADELBROT
#SBATCH -o MPI_MANDELBROT.%J.stdout
#SBATCH -e MPI_MANDELBROT.%J.stderr
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

max=65
num_proc=101

chunk=2
max_chunk=10
while [ "$chunk" -lt "$max_chunk" ] 
do
    mpirun -n 4 ./mandelbrot -t $chunk -m 0 >> "stats_mpi_mandelbrot_static_chunk_4_.csv"
    chunk=$(($chunk+1))
done

chunk=2
while [ "$chunk" -lt "$max_chunk" ] 
do
    mpirun -n 8 ./mandelbrot -t $chunk -m 0 >> "stats_mpi_mandelbrot_static_chunk_8_.csv"
    chunk=$(($chunk+1))
done

chunk=2
while [ "$chunk" -lt "$max_chunk" ] 
do
    mpirun -n 16 ./mandelbrot -t $chunk -m 0 >> "stats_mpi_mandelbrot_static_chunk_16_.csv"
    chunk=$(($chunk+1))
done

num_proc=2
while [ "$num_proc" -lt "$max" ] 
do
    mpirun -n $num_proc ./mandelbrot -t 10 -m 0 >> "stats_mpi_mandelbrot_static.csv"
    num_proc=$(($num_proc+1))
done

num_proc=2
while [ "$num_proc" -lt "$max" ] 
do
    mpirun -n $num_proc ./mandelbrot -m 1 >> "stats_mpi_mandelbrot_dynamic.csv"
    num_proc=$(($num_proc+1))
done
:

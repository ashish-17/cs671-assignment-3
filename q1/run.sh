#!/bin/bash
#SBATCH -J MPI_PINGPONG
#SBATCH -o MPI_PINGPONG.%J.stdout
#SBATCH -e MPI_PINGPONG.%J.stderr
#SBATCH -p main
#SBATCH --reservation mp002
#SBATCH -N 1
#SBATCH -t 00:10:00

#Uncomment following lines before running on caliburn
#cd $HOME/q1
#module load openmpi
#sleep 3

set -e

make clean

find . -type f -name '*.csv' -delete

make

num_bytes=1
max=$((1<<21))
while [ "$num_bytes" -lt "$max" ] 
do
    mpirun -n 2 ./main $num_bytes 10000 10000000 >> "stats.csv"
    num_bytes=$(($num_bytes<<1))
done

:

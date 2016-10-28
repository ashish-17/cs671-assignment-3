#!/bin/bash

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

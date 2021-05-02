#!/bin/bash

declare -a arrRandom_control=("arbiter" "cavlc" "ctrl" "dec" "i2c" "int2float" "mem_ctrl" "priority" "router" "voter" )
declare -a arrArithmetic=("adder" "bar" "div" "hyp" "log2" "max" "multiplier" "sin" "sqrt" "square" )

for val in ${arrRandom_control[@]}; do
   ./abc -c "gencareset -n 3000 ../benchmarks/EPFL-benchmark-suite/random_control/$val.blif careset/$val.3000.c00.v01.careset.pla samples/$val.3000.c00.v01"
done

for val in ${arrArithmetic[@]}; do
   ./abc -c "gencareset -n 3000 ../benchmarks/EPFL-benchmark-suite/arithmetic/$val.blif careset/$val.3000.c00.v01.careset.pla samples/$val.3000.c00.v01"
done


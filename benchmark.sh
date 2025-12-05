#!/bin/bash

# Output file
OUTPUT_FILE="benchmark_results.csv"
echo "Type,Threads,Steps,Width,Height,TotalTime,TimePerStep" > $OUTPUT_FILE

# Benchmark Parameters
STEPS=1000
SIZES="500 1000 2000 5000 10000 20000"
THREADS="1 2 4 8 16"

echo "Starting Benchmarks..."

# 1. Run Serial Benchmark
echo "Running Serial Benchmarks..."
for s in $SIZES; do
    echo "  - Grid: $s x $s"
    # Run simulation, capture stderr (2>) to process CSV data, let stdout (1) go to terminal
    ./sir_sim $STEPS $s $s 2>&1 >/dev/tty | grep "CSV_DATA" | sed "s/CSV_DATA,/Serial,1,/" >> $OUTPUT_FILE
done

# 2. Run OpenMP Benchmark
echo "Running OpenMP Benchmarks..."
for t in $THREADS; do
    export OMP_NUM_THREADS=$t
    echo "  - Threads: $t"
    for s in $SIZES; do
        echo "      - Grid: $s x $s"
        # Run simulation, capture stderr (2>) to process CSV data, let stdout (1) go to terminal
        ./sir_sim_omp $STEPS $s $s 2>&1 >/dev/tty | grep "CSV_DATA" | sed "s/CSV_DATA,/OpenMP,$t,/" >> $OUTPUT_FILE
    done
done

echo "Benchmarking Complete! Results saved to $OUTPUT_FILE"

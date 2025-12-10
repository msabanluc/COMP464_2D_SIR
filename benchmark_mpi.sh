#!/bin/bash

# Output file
OUTPUT_FILE="benchmark_results_mpi_nobuff_hostfix.csv"

# Benchmark Parameters
STEPS=1000
SIZES="500 1000 2000 5000 10000 20000"
PROCS="1 2 4 8 16 32 64"

# Run MPI Benchmark
echo "Running MPI Benchmarks no buffer..."
for p in $PROCS; do
    echo "  - Processes: $p"
    for s in $SIZES; do
        echo "      - Grid: $s x $s"
        mpirun $MPI_ARGS -np $p -hostfile my-hosts-64 ./sir_sim_mpi_nobuff $STEPS $s $s | tee /dev/tty | grep "CSV_DATA" | sed "s/CSV_DATA,/MPI,$p,/" >> $OUTPUT_FILE
    done
done

echo "MPI Benchmarking Complete! Results appended to $OUTPUT_FILE"

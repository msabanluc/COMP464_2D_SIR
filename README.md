# SIR Model HPC Simulation

This project implements a simulation of disease spread using the SIR (Susceptible-Infectious-Resistant) Cellular Automata model, specifically to test HPC techniques to simulate large grids efficiently.

This project is based on the original implementation by [Victor Gordan](https://github.com/VictorGordan/sir). We started by cloning their repository and extending it for HPC benchmarking with various parallelization strategies.


The project includes several implementations to explore different parallelization strategies:

1.  **Serial (`sir_sim`)**: A baseline serial implementation
2.  **OpenMP (`sir_sim_omp`)**: Shared memory optimization using OpenMP
3.  **MPI (`sir_sim_mpi`)**: Distributed memory optimization using MPI
4.  **MPI No Buffer (`sir_sim_mpi_nobuff`)**: MPI with non-blocking communication
5.  **Hybrid (`sir_sim_hybrid`)**: Combines MPI and OpenMP for both distributed and shared memory parallelism



## Running the Simulation

Each executable takes the following command-line arguments:
```bash
./<executable> <steps> <width> <height>
```

-   `steps`: Number of simulation steps.
-   `width`: Width of the grid.
-   `height`: Height of the grid.


## Visualization

<video src="presentation/sir_simulation.mp4" controls="controls" style="max-width: 100%;">
</video>


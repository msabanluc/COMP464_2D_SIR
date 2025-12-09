#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <random>
#include <algorithm>
#include <omp.h>

// Default simulation properties
const int DEFAULT_WIDTH = 500;
const int DEFAULT_HEIGHT = 500;
const int DEFAULT_STEPS = 1000;

// SIR Parameters (could add these as arguments later, we should play around with these to make sure we get interesting results)
float initialInfectious = 0.01f; // 1% initial infection
int infectiousTime = 10; // Infected cell remains infectious for 10 time steps
int resistantTime = 50; // Resistant cell remains resistant for 50 time steps
int seed = 1;

enum State : uint8_t {
    Susceptible = 0,
    Infectious = 1,
    Resistant = 2
};

// Structure of arrays for simulation data
struct SimulationData {
    int width;
    int height;
    std::vector<uint8_t> state;      // Current state
    std::vector<uint8_t> next_state; // Next state (buffer)
    std::vector<float> popDensity;   // Population density [0.0, 1.0]
    std::vector<int> time;           // Time in current state
    std::vector<uint32_t> rng_state; // Per-cell RNG state
};

// Random numbers - LCG for per-cell RNG 
// We could see how simple rand() performs too for serial approach or benchmark with/without in parallel.
// Can also try other approaches. Not sure how much RNG will impact performance here).
inline float fast_rand(uint32_t& state) {
    state = state * 1664525 + 1013904223;
    return (float)state / (float)0xFFFFFFFF;
}

// Initialize simulation data
void initialize(SimulationData& sim, int w, int h) {
    sim.width = w;
    sim.height = h;
    size_t size = w * h;
    
    sim.state.resize(size);
    sim.next_state.resize(size);
    sim.popDensity.resize(size);
    sim.time.resize(size, 0);
    sim.rng_state.resize(size);

    // Initialize per-cell RNG states with Mersenne Twister
    std::mt19937 gen(seed);
    for (size_t i = 0; i < size; ++i) {
        sim.rng_state[i] = gen();
    }

    // Initialize with synthetic data
    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < size; ++i) {
        // Set row and column indexes
        int r = i / w;
        int c = i % w;
        // Smooth density pattern using sin(row) and cos(col) patterns
        float val = (std::sin(r * 0.05f) + std::cos(c * 0.05f) + 2.0f) / 4.0f;
        // Add some noise
        val += (fast_rand(sim.rng_state[i]) - 0.5f) * 0.2f;

        // Island Mask
        // Normalize coordinates to -1.0 to 1.0 regardless of map size
        float nx = (2.0f * c / w) - 1.0f;
        float ny = (2.0f * r / h) - 1.0f;
        float dist = std::sqrt(nx*nx + ny*ny);
        
        // Create circular mask
        float islandMask = 1.0f - std::pow(dist, 2.0f); 
        if (islandMask < 0.0f) islandMask = 0.0f;
        val *= islandMask;

        // Partial River
        if (r > h / 2) {
            // Scale river width to be 1.5% of the map width
            float riverWidth = w * 0.015f;
            if (riverWidth < 1.0f) riverWidth = 1.0f; // Minimum 1 pixel

            // Calculate path using normalized height (0.5 to 1.0)
            float normR = (float)r / h; 
            
            // Sine wave for the river path. 
            float centerOffset = (w * 0.1f) * std::sin(normR * 10.0f);
            float riverCenter = (w / 2.0f) + centerOffset;

            // Carve the river
            if (std::abs(c - riverCenter) < riverWidth) {
                val = 0.0f; // Uninhabited (Water)
            }
        }

        // Constrain to [0.0, 1.0]
        val = std::max(0.0f, std::min(1.0f, val));
        
        sim.popDensity[i] = val;

        // Initialize state based on initialInfectious probability
        if (val == 0.0f) {
            sim.state[i] = Resistant; // Uninhabited areas are permanently resistant (water/empty)
        } else if (fast_rand(sim.rng_state[i]) < initialInfectious) {
            sim.state[i] = Infectious; // If population density > 0, small chance to start in Infectious state
        } else {
            sim.state[i] = Susceptible; // Otherwise start in Susceptible state
        }
        sim.next_state[i] = sim.state[i]; // Initialize next_state to current state
    }
}

// Check 8 neighbors and return count of Infectious cells
inline int checkInfectious(const SimulationData& sim, int r, int c) {
    int w = sim.width;
    int h = sim.height;
    int count = 0;
    
    for (int dr = -1; dr <= 1; ++dr) { // Loop over neighbor rows
        for (int dc = -1; dc <= 1; ++dc) { // Loop over neighbor columns (Creates a 3x3 neighborhood with center at (r,c))
            if (dr == 0 && dc == 0) continue; // Skip the center cell itself
            
            // Calculate neighbor coordinates
            int nr = r + dr;
            int nc = c + dc;
            
            if (nr >= 0 && nr < h && nc >= 0 && nc < w) { // Make sure neighbor is within bounds of the grid
                if (sim.state[nr * w + nc] == Infectious) {
                    count++;
                }
            }
        }
    }
    return count;
}

// Update simulation state for one time step
void update(SimulationData& sim) {
    int w = sim.width;
    int h = sim.height;
    
    #pragma omp parallel for collapse(2) schedule(static)
    for (int r = 0; r < h; ++r) {
        for (int c = 0; c < w; ++c) {
            int idx = r * w + c;
            uint8_t s = sim.state[idx];
            float density = sim.popDensity[idx];
            
            if (s == Susceptible) {
                int infectedNeighbors = checkInfectious(sim, r, c);
                if (density > 0.0f && infectedNeighbors > 0) { // If a cell is susceptible, has population density > 0, and has at least one infectious neighbor, state may change to infectious
                    float baseProb = (0.6f / (1.0f + 1800.0f * std::exp(-15.0f * density))) + 0.1f; // Base infection probability based on density
                    
                    float prob = 1.0f - std::pow(1.0f - baseProb, (float)infectedNeighbors); // Adjust probability based on number of infected neighbors: 1 - (1 - p)^k

                    if (fast_rand(sim.rng_state[idx]) < prob) { // Infection occurs based on probability
                        sim.next_state[idx] = Infectious;
                        sim.time[idx] = 0; // Reset time counter on state change
                    } else {
                        sim.next_state[idx] = Susceptible; // Remain susceptible if infection does not occur
                    }
                } else {
                    sim.next_state[idx] = Susceptible; // Remain susceptible if no infectious neighbors or zero population density
                }
            } else if (s == Infectious) {
                if (sim.time[idx] >= infectiousTime) { // Become resistant after infectiousTime steps
                    sim.next_state[idx] = Resistant;
                    sim.time[idx] = 0;
                } else {
                    sim.next_state[idx] = Infectious; // Remain infectious if infectiousTime not reached
                    sim.time[idx]++;
                }
            } else if (s == Resistant) {
                if (density == 0.0f) {
                    sim.next_state[idx] = Resistant; // Permanently resistant (water/empty)
                    sim.time[idx] = 0;
                } else if (sim.time[idx] >= resistantTime) { // Become susceptible after resistantTime steps
                    sim.next_state[idx] = Susceptible;
                    sim.time[idx] = 0;
                } else {
                    sim.next_state[idx] = Resistant; // Remain resistant if resistantTime not reached
                    sim.time[idx]++;
                }
            }
        }
    }
    
    // Swap buffers
    std::swap(sim.state, sim.next_state);
}

// Print statistics
void print_stats(const SimulationData& sim, int step) {
    long long sus = 0, inf = 0, res = 0;
    size_t size = sim.state.size();
    
    #pragma omp parallel for reduction(+:sus, inf, res)
    for (size_t i = 0; i < size; ++i) {
        uint8_t s = sim.state[i];
        if (s == Susceptible) sus++;
        else if (s == Infectious) inf++;
        else if (s == Resistant) res++;
    }
    std::cout << "Step " << step << ": S=" << sus << " I=" << inf << " R=" << res << "\n";
}

int main(int argc, char** argv) {
    int steps = DEFAULT_STEPS;
    int width = DEFAULT_WIDTH;
    int height = DEFAULT_HEIGHT;

    // Parse command-line arguments
    if (argc > 1) steps = std::atoi(argv[1]);
    if (argc > 2) width = std::atoi(argv[2]);
    if (argc > 3) height = std::atoi(argv[3]);
    if (argc > 4) initialInfectious = std::atof(argv[4]);
    if (argc > 5) infectiousTime = std::atoi(argv[5]);
    if (argc > 6) resistantTime = std::atoi(argv[6]);

    std::cout << "Initializing SIR Simulation (" << width << "x" << height << ") for " << steps << " steps...\n";

    SimulationData sim; // Create empty simulation data structure
    
    auto init_start = std::chrono::high_resolution_clock::now();
    initialize(sim, width, height); // Initialize simulation data with synthetic values
    auto init_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> init_elapsed = init_end - init_start;
    std::cout << "Initialization complete in " << init_elapsed.count() << " seconds.\n";

    print_stats(sim, 0); // Print initial stats

    auto start_time = std::chrono::high_resolution_clock::now(); // Start timing

    for (int i = 1; i <= steps; ++i) { // Loop over simulation steps
        update(sim);
        if (i % 100 == 0) { // Print stats every 100 steps
            print_stats(sim, i); 
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now(); // End timing
    
    std::chrono::duration<double> elapsed = end_time - start_time;

    std::cout << "Simulation complete.\n";
    std::cout << "Time elapsed: " << elapsed.count() << " seconds\n";
    std::cout << "Average time per step: " << (elapsed.count() / steps) * 1000.0 << " ms\n";
    
    // CSV Output: Steps, Width, Height, TotalTime(s), TimePerStep(ms)
    std::cerr << "CSV_DATA," << steps << "," << width << "," << height << "," 
              << elapsed.count() << "," << (elapsed.count() / steps) * 1000.0 << "\n";

    print_stats(sim, steps);

    return 0;
}

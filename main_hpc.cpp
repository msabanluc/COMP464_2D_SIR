#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <random>
#include <algorithm>

// Simulation properties
const int DEFAULT_WIDTH = 500;
const int DEFAULT_HEIGHT = 500;
const int DEFAULT_STEPS = 1000;

// SIR Parameters
float initialInfectious = 0.01f; // 1% initial infection
int infectiousTime = 14;
int resistantTime = 240;
int seed = 1;

enum State : uint8_t {
    Susceptible = 0,
    Infectious = 1,
    Resistant = 2
};

// Structure of Arrays (SoA) layout for better cache locality and vectorization potential
struct SimulationData {
    int width;
    int height;
    std::vector<uint8_t> state;      // Current state
    std::vector<uint8_t> next_state; // Next state (double buffering)
    std::vector<float> popDensity;   // Population density (0.0 - 1.0)
    std::vector<int> time;           // Time in current state
    std::vector<uint32_t> rng_state; // Per-cell RNG state
};

// Random numbers
inline float fast_rand(uint32_t& state) {
    state = state * 1664525 + 1013904223;
    return (float)state / (float)0xFFFFFFFF;
}

void initialize(SimulationData& sim, int w, int h) {
    sim.width = w;
    sim.height = h;
    size_t size = w * h;
    
    sim.state.resize(size);
    sim.next_state.resize(size);
    sim.popDensity.resize(size);
    sim.time.resize(size, 0);
    sim.rng_state.resize(size);

    // Initialize RNG states
    std::mt19937 gen(seed);
    for (size_t i = 0; i < size; ++i) {
        sim.rng_state[i] = gen();
    }

    // Initialize with synthetic data
    for (size_t i = 0; i < size; ++i) {
        // Generate a density between 0 and 1
        int r = i / w;
        int c = i % w;
        float val = (std::sin(r * 0.05f) + std::cos(c * 0.05f) + 2.0f) / 4.0f;
        // Add some noise
        val += (fast_rand(sim.rng_state[i]) - 0.5f) * 0.2f;
        val = std::max(0.0f, std::min(1.0f, val));
        
        sim.popDensity[i] = val;

        if (val > 0.0f && fast_rand(sim.rng_state[i]) < initialInfectious) {
            sim.state[i] = Infectious;
        } else {
            sim.state[i] = Susceptible;
        }
        sim.next_state[i] = sim.state[i];
    }
}

// Check 8 neighbors for any Infectious cell
inline bool checkInfectious(const SimulationData& sim, int r, int c) {
    int w = sim.width;
    int h = sim.height;
    
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue;
            
            int nr = r + dr;
            int nc = c + dc;
            
            if (nr >= 0 && nr < h && nc >= 0 && nc < w) {
                if (sim.state[nr * w + nc] == Infectious) {
                    return true;
                }
            }
        }
    }
    return false;
}

void update(SimulationData& sim) {
    int w = sim.width;
    int h = sim.height;
    
    for (int r = 0; r < h; ++r) {
        for (int c = 0; c < w; ++c) {
            int idx = r * w + c;
            uint8_t s = sim.state[idx];
            float density = sim.popDensity[idx];
            
            if (s == Susceptible) {
                if (density > 0.0f && checkInfectious(sim, r, c)) {
                    // Infection probability formula
                    float prob = (0.8f / (1.0f + 1800.0f * std::exp(-15.0f * density))) + 0.1f;
                    if (fast_rand(sim.rng_state[idx]) < prob) {
                        sim.next_state[idx] = Infectious;
                        sim.time[idx] = 0;
                    } else {
                        sim.next_state[idx] = Susceptible;
                    }
                } else {
                    sim.next_state[idx] = Susceptible;
                }
            } else if (s == Infectious) {
                if (sim.time[idx] >= infectiousTime) {
                    sim.next_state[idx] = Resistant;
                    sim.time[idx] = 0;
                } else {
                    sim.next_state[idx] = Infectious;
                    sim.time[idx]++;
                }
            } else if (s == Resistant) {
                if (sim.time[idx] >= resistantTime) {
                    sim.next_state[idx] = Susceptible;
                    sim.time[idx] = 0;
                } else {
                    sim.next_state[idx] = Resistant;
                    sim.time[idx]++;
                }
            }
        }
    }
    
    // Swap buffers
    std::swap(sim.state, sim.next_state);
}

void print_stats(const SimulationData& sim, int step) {
    long long sus = 0, inf = 0, res = 0;
    for (uint8_t s : sim.state) {
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

    if (argc > 1) steps = std::atoi(argv[1]);
    if (argc > 2) width = std::atoi(argv[2]);
    if (argc > 3) height = std::atoi(argv[3]);

    std::cout << "Initializing SIR Simulation (" << width << "x" << height << ") for " << steps << " steps...\n";

    SimulationData sim;
    initialize(sim, width, height);

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < steps; ++i) {
        update(sim);
        if (i % 100 == 0) {
            // print_stats(sim, i); 
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    std::cout << "Simulation complete.\n";
    std::cout << "Time elapsed: " << elapsed.count() << " seconds\n";
    std::cout << "Average time per step: " << (elapsed.count() / steps) * 1000.0 << " ms\n";
    
    // CSV Output: Steps, Width, Height, TotalTime(s), TimePerStep(ms)
    std::cout << "CSV_DATA," << steps << "," << width << "," << height << "," 
              << elapsed.count() << "," << (elapsed.count() / steps) * 1000.0 << "\n";

    print_stats(sim, steps);

    return 0;
}

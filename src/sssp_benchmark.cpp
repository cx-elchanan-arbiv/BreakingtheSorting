/**
 * SSSP Benchmark Tool
 *
 * Compares the new O(m log^{2/3} n) algorithm with LEMON's Dijkstra
 * on graphs loaded from MTX files.
 *
 * Usage: ./sssp_benchmark <path_to_mtx_file> [num_runs] [source_node]
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cmath>

#include "mtx_parser.hpp"
#include "new_sssp.hpp"
#include "dijkstra_lemon.hpp"

using namespace sssp;

// Statistics helper
struct BenchmarkStats {
    double mean;
    double median;
    double std_dev;
    double min;
    double max;

    static BenchmarkStats compute(std::vector<double>& times) {
        BenchmarkStats stats;

        if (times.empty()) {
            return {0, 0, 0, 0, 0};
        }

        std::sort(times.begin(), times.end());

        stats.min = times.front();
        stats.max = times.back();
        stats.median = times[times.size() / 2];

        double sum = std::accumulate(times.begin(), times.end(), 0.0);
        stats.mean = sum / times.size();

        double sq_sum = 0;
        for (double t : times) {
            sq_sum += (t - stats.mean) * (t - stats.mean);
        }
        stats.std_dev = std::sqrt(sq_sum / times.size());

        return stats;
    }
};

void printUsage(const char* program) {
    std::cerr << "Usage: " << program << " <mtx_file> [num_runs] [source_node]\n";
    std::cerr << "\n";
    std::cerr << "Arguments:\n";
    std::cerr << "  mtx_file     Path to Matrix Market (.mtx) graph file\n";
    std::cerr << "  num_runs     Number of benchmark runs (default: 5)\n";
    std::cerr << "  source_node  Source node for SSSP (default: 0)\n";
    std::cerr << "\n";
    std::cerr << "Example:\n";
    std::cerr << "  " << program << " /path/to/graph.mtx 10 0\n";
}

void printSeparator(char c = '=', int width = 70) {
    std::cout << std::string(width, c) << "\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string mtx_path = argv[1];
    int num_runs = (argc > 2) ? std::atoi(argv[2]) : 5;
    int source = (argc > 3) ? std::atoi(argv[3]) : 0;

    if (num_runs < 1) num_runs = 1;

    // Print header
    std::cout << "\n";
    printSeparator();
    std::cout << "  SSSP Benchmark: New Algorithm vs LEMON Dijkstra\n";
    printSeparator();
    std::cout << "\n";

    // Load graph
    std::cout << "Loading graph from: " << mtx_path << "\n";

    SimpleGraph graph;
    MTXParser::GraphInfo info;

    try {
        auto result = MTXParser::parse(mtx_path);
        graph = std::move(result.first);
        info = result.second;
    } catch (const std::exception& e) {
        std::cerr << "Error loading graph: " << e.what() << "\n";
        return 1;
    }

    // Validate source
    if (source < 0 || source >= graph.n) {
        std::cerr << "Invalid source node: " << source << " (graph has " << graph.n << " nodes)\n";
        return 1;
    }

    // Print graph info
    std::cout << "\n";
    printSeparator('-');
    std::cout << "Graph Statistics:\n";
    printSeparator('-');
    std::cout << "  Nodes:        " << std::setw(15) << graph.n << "\n";
    std::cout << "  Edges:        " << std::setw(15) << graph.m << "\n";
    std::cout << "  Density:      " << std::setw(15) << std::fixed << std::setprecision(6)
              << (double)graph.m / ((double)graph.n * graph.n) << "\n";
    std::cout << "  Avg degree:   " << std::setw(15) << std::setprecision(2)
              << (double)graph.m / graph.n << "\n";
    std::cout << "  Type:         " << std::setw(15) << (info.is_directed ? "Directed" : "Undirected") << "\n";
    std::cout << "  Source node:  " << std::setw(15) << source << "\n";
    std::cout << "  Benchmark runs:" << std::setw(14) << num_runs << "\n";

    // Theoretical complexity comparison
    double n = graph.n;
    double m = graph.m;
    double log_n = std::log2(n);
    double dijkstra_complexity = m + n * log_n;
    double new_complexity = m * std::pow(log_n, 2.0/3.0);

    std::cout << "\n";
    printSeparator('-');
    std::cout << "Theoretical Complexity:\n";
    printSeparator('-');
    std::cout << "  Dijkstra:     O(m + n log n) = O(" << std::scientific << std::setprecision(2)
              << dijkstra_complexity << ")\n";
    std::cout << "  New SSSP:     O(m log^{2/3} n) = O(" << new_complexity << ")\n";
    std::cout << "  Ratio:        " << std::fixed << std::setprecision(3)
              << dijkstra_complexity / new_complexity << "x (theoretical)\n";

    // Warmup run
    std::cout << "\n";
    printSeparator('-');
    std::cout << "Running warmup...\n";
    printSeparator('-');

    {
        auto dijkstra_result = DijkstraLemon::solve(graph, source);
        NewSSSP solver(graph);
        auto new_result = solver.solve(source);

        // Verify correctness
        int mismatches = 0;
        double max_error = 0;
        int dijkstra_reachable = 0;
        int new_reachable = 0;

        for (int i = 0; i < graph.n; ++i) {
            if (dijkstra_result.distances[i] < INF) {
                dijkstra_reachable++;
                if (new_result.distances[i] < INF) {
                    double error = std::abs(dijkstra_result.distances[i] - new_result.distances[i]);
                    max_error = std::max(max_error, error);
                    if (error > 1e-6) {
                        mismatches++;
                    }
                } else {
                    mismatches++;
                }
            }
            if (new_result.distances[i] < INF) {
                new_reachable++;
            }
        }

        std::cout << "  Dijkstra reachable: " << dijkstra_reachable << " / " << graph.n << "\n";
        std::cout << "  New SSSP reachable: " << new_reachable << " / " << graph.n << "\n";
        std::cout << "  Max error:          " << std::scientific << max_error << "\n";
        std::cout << "  Correctness:        " << (mismatches == 0 ? "PASSED" : "FAILED") << "\n";

        if (mismatches > 0) {
            std::cerr << "  WARNING: " << mismatches << " distance mismatches detected!\n";
        }
    }

    // Benchmark runs
    std::cout << "\n";
    printSeparator('-');
    std::cout << "Running benchmark (" << num_runs << " iterations)...\n";
    printSeparator('-');

    std::vector<double> dijkstra_times;
    std::vector<double> lemon_times;
    std::vector<double> new_times;

    for (int run = 0; run < num_runs; ++run) {
        std::cout << "  Run " << (run + 1) << "/" << num_runs << "... " << std::flush;

        // Simple Dijkstra (our implementation)
        auto start = std::chrono::high_resolution_clock::now();
        auto dijkstra_result = SimpleDijkstra::solve(graph, source);
        auto end = std::chrono::high_resolution_clock::now();
        double dijkstra_ms = std::chrono::duration<double, std::milli>(end - start).count();
        dijkstra_times.push_back(dijkstra_ms);

        // LEMON Dijkstra
        start = std::chrono::high_resolution_clock::now();
        auto lemon_result = DijkstraLemon::solve(graph, source);
        end = std::chrono::high_resolution_clock::now();
        double lemon_ms = std::chrono::duration<double, std::milli>(end - start).count();
        lemon_times.push_back(lemon_ms);

        // New SSSP
        start = std::chrono::high_resolution_clock::now();
        NewSSSP solver(graph);
        auto new_result = solver.solve(source);
        end = std::chrono::high_resolution_clock::now();
        double new_ms = std::chrono::duration<double, std::milli>(end - start).count();
        new_times.push_back(new_ms);

        std::cout << "Dijkstra: " << std::fixed << std::setprecision(2) << dijkstra_ms
                  << "ms, LEMON: " << lemon_ms
                  << "ms, New: " << new_ms << "ms\n";
    }

    // Compute statistics
    auto dijkstra_stats = BenchmarkStats::compute(dijkstra_times);
    auto lemon_stats = BenchmarkStats::compute(lemon_times);
    auto new_stats = BenchmarkStats::compute(new_times);

    // Print results
    std::cout << "\n";
    printSeparator('=');
    std::cout << "  BENCHMARK RESULTS\n";
    printSeparator('=');

    std::cout << "\n";
    std::cout << std::left << std::setw(20) << "Algorithm"
              << std::right << std::setw(12) << "Mean (ms)"
              << std::setw(12) << "Median"
              << std::setw(12) << "Std Dev"
              << std::setw(12) << "Min"
              << std::setw(12) << "Max" << "\n";
    printSeparator('-');

    auto printRow = [](const std::string& name, const BenchmarkStats& stats) {
        std::cout << std::left << std::setw(20) << name
                  << std::right << std::fixed << std::setprecision(2)
                  << std::setw(12) << stats.mean
                  << std::setw(12) << stats.median
                  << std::setw(12) << stats.std_dev
                  << std::setw(12) << stats.min
                  << std::setw(12) << stats.max << "\n";
    };

    printRow("Dijkstra (simple)", dijkstra_stats);
    printRow("LEMON Dijkstra", lemon_stats);
    printRow("New SSSP", new_stats);

    // Speedup comparison
    std::cout << "\n";
    printSeparator('-');
    std::cout << "Speedup Analysis (based on median times):\n";
    printSeparator('-');

    double dijkstra_vs_new = dijkstra_stats.median / new_stats.median;
    double lemon_vs_new = lemon_stats.median / new_stats.median;

    std::cout << "  Dijkstra / New SSSP: " << std::fixed << std::setprecision(3)
              << dijkstra_vs_new << "x";
    if (dijkstra_vs_new > 1) {
        std::cout << " (New SSSP is faster)";
    } else {
        std::cout << " (Dijkstra is faster)";
    }
    std::cout << "\n";

    std::cout << "  LEMON / New SSSP:    " << lemon_vs_new << "x";
    if (lemon_vs_new > 1) {
        std::cout << " (New SSSP is faster)";
    } else {
        std::cout << " (LEMON is faster)";
    }
    std::cout << "\n";

    // Summary
    std::cout << "\n";
    printSeparator('=');
    std::cout << "  SUMMARY\n";
    printSeparator('=');
    std::cout << "\n";
    std::cout << "  Graph:          " << mtx_path << "\n";
    std::cout << "  Size:           " << graph.n << " nodes, " << graph.m << " edges\n";
    std::cout << "  Best time:      ";

    if (new_stats.median <= dijkstra_stats.median && new_stats.median <= lemon_stats.median) {
        std::cout << "New SSSP (" << new_stats.median << " ms)\n";
    } else if (lemon_stats.median <= dijkstra_stats.median) {
        std::cout << "LEMON Dijkstra (" << lemon_stats.median << " ms)\n";
    } else {
        std::cout << "Simple Dijkstra (" << dijkstra_stats.median << " ms)\n";
    }

    std::cout << "\n";

    return 0;
}

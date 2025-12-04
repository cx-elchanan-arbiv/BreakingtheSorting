#include <iostream>
#include <iomanip>
#include <chrono>
#include "new_sssp.hpp"
#include "dijkstra_lemon.hpp"
#include "graph_generator.hpp"

using namespace sssp;

void printResults(const std::string& name, const std::vector<double>& distances,
                  double time_ms, int n) {
    std::cout << "\n" << name << " Results:\n";
    std::cout << "  Time: " << std::fixed << std::setprecision(3) << time_ms << " ms\n";

    // Show first few distances
    std::cout << "  First 10 distances: ";
    for (int i = 0; i < std::min(10, n); ++i) {
        if (distances[i] < INF) {
            std::cout << std::setprecision(2) << distances[i] << " ";
        } else {
            std::cout << "INF ";
        }
    }
    std::cout << "\n";

    // Count reachable
    int reachable = 0;
    double max_dist = 0;
    for (int i = 0; i < n; ++i) {
        if (distances[i] < INF) {
            reachable++;
            max_dist = std::max(max_dist, distances[i]);
        }
    }
    std::cout << "  Reachable nodes: " << reachable << "/" << n << "\n";
    std::cout << "  Max distance: " << max_dist << "\n";
}

void runComparison(int n, int m, const std::string& graph_type) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Graph: " << graph_type << " (n=" << n << ", m=" << m << ")\n";
    std::cout << std::string(60, '=') << "\n";

    SimpleGraph g;
    if (graph_type == "sparse") {
        g = GraphGenerator::randomSparse(n, m);
    } else if (graph_type == "grid") {
        int side = static_cast<int>(std::sqrt(n));
        g = GraphGenerator::grid(side, side);
    } else if (graph_type == "scalefree") {
        g = GraphGenerator::scaleFree(n, 5, 3);
    } else {
        g = GraphGenerator::randomSparse(n, m);
    }

    std::cout << "Graph created: " << g.n << " nodes, " << g.m << " edges\n";

    // Run Dijkstra
    auto start = std::chrono::high_resolution_clock::now();
    auto dijkstra_result = SimpleDijkstra::solve(g, 0);
    auto end = std::chrono::high_resolution_clock::now();
    double dijkstra_time = std::chrono::duration<double, std::milli>(end - start).count();
    printResults("Dijkstra", dijkstra_result.distances, dijkstra_time, g.n);

    // Run LEMON Dijkstra
    start = std::chrono::high_resolution_clock::now();
    auto lemon_result = DijkstraLemon::solve(g, 0);
    end = std::chrono::high_resolution_clock::now();
    double lemon_time = std::chrono::duration<double, std::milli>(end - start).count();
    printResults("LEMON Dijkstra", lemon_result.distances, lemon_time, g.n);

    // Run New SSSP
    start = std::chrono::high_resolution_clock::now();
    NewSSSP solver(g);
    auto new_result = solver.solve(0);
    end = std::chrono::high_resolution_clock::now();
    double new_time = std::chrono::duration<double, std::milli>(end - start).count();
    printResults("New SSSP (O(m log^{2/3} n))", new_result.distances, new_time, g.n);

    // Verify correctness
    std::cout << "\nVerifying correctness...\n";
    bool correct = true;
    double max_error = 0;
    for (int i = 0; i < g.n; ++i) {
        if (dijkstra_result.distances[i] < INF) {
            double error = std::abs(dijkstra_result.distances[i] - new_result.distances[i]);
            max_error = std::max(max_error, error);
            if (error > 1e-6) {
                correct = false;
            }
        }
    }
    std::cout << "  Correctness: " << (correct ? "PASSED" : "FAILED") << "\n";
    std::cout << "  Max error: " << std::scientific << max_error << "\n";

    // Speedup
    std::cout << "\nPerformance comparison:\n";
    std::cout << "  Dijkstra/New ratio: " << std::fixed << std::setprecision(2)
              << dijkstra_time / new_time << "x\n";
    std::cout << "  LEMON/New ratio: " << lemon_time / new_time << "x\n";
}

int main(int argc, char* argv[]) {
    std::cout << "Breaking the Sorting Barrier for Directed SSSP\n";
    std::cout << "Implementation based on Duan et al. (2025)\n";
    std::cout << "O(m log^{2/3} n) vs O(m + n log n) Dijkstra\n\n";

    // Default parameters
    std::vector<std::pair<int, int>> sizes = {
        {1000, 2000},
        {10000, 20000},
        {100000, 200000},
        {500000, 1000000},
    };

    if (argc > 1) {
        // Custom size
        int n = std::atoi(argv[1]);
        int m = argc > 2 ? std::atoi(argv[2]) : n * 2;
        sizes = {{n, m}};
    }

    for (const auto& [n, m] : sizes) {
        runComparison(n, m, "sparse");

        // Skip grid for very large sizes (would be huge)
        if (n <= 100000) {
            int side = static_cast<int>(std::sqrt(n));
            runComparison(side * side, 0, "grid");
        }
    }

    // Run on a scale-free graph
    std::cout << "\n\nScale-free graph benchmark:\n";
    runComparison(100000, 0, "scalefree");

    return 0;
}

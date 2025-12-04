#include <benchmark/benchmark.h>
#include "new_sssp.hpp"
#include "dijkstra_lemon.hpp"
#include "graph_generator.hpp"
#include <memory>

using namespace sssp;

// ============================================================================
// Benchmark fixtures for different graph types and sizes
// ============================================================================

// Global graph storage to avoid regenerating graphs for each benchmark iteration
namespace {
    std::map<std::string, SimpleGraph> graph_cache;

    SimpleGraph& getOrCreateGraph(const std::string& key, int n, int m, int seed) {
        if (graph_cache.find(key) == graph_cache.end()) {
            graph_cache[key] = GraphGenerator::randomSparse(n, m, 1.0, 100.0, seed);
        }
        return graph_cache[key];
    }

    SimpleGraph& getOrCreateGrid(const std::string& key, int rows, int cols, int seed) {
        if (graph_cache.find(key) == graph_cache.end()) {
            graph_cache[key] = GraphGenerator::grid(rows, cols, 1.0, 10.0, seed);
        }
        return graph_cache[key];
    }

    SimpleGraph& getOrCreateScaleFree(const std::string& key, int n, int seed) {
        if (graph_cache.find(key) == graph_cache.end()) {
            graph_cache[key] = GraphGenerator::scaleFree(n, 5, 3, 1.0, 100.0, seed);
        }
        return graph_cache[key];
    }
}

// ============================================================================
// Sparse Random Graphs - Main comparison (where the paper shows improvement)
// ============================================================================

static void BM_Dijkstra_Sparse(benchmark::State& state) {
    int n = state.range(0);
    int m = n * 2;  // Sparse: m = O(n)
    std::string key = "sparse_" + std::to_string(n) + "_" + std::to_string(m);
    auto& g = getOrCreateGraph(key, n, m, 42);

    for (auto _ : state) {
        auto result = SimpleDijkstra::solve(g, 0);
        benchmark::DoNotOptimize(result);
    }

    state.SetComplexityN(n);
    state.counters["nodes"] = n;
    state.counters["edges"] = g.m;
}

static void BM_NewSSSP_Sparse(benchmark::State& state) {
    int n = state.range(0);
    int m = n * 2;
    std::string key = "sparse_" + std::to_string(n) + "_" + std::to_string(m);
    auto& g = getOrCreateGraph(key, n, m, 42);

    for (auto _ : state) {
        NewSSSP solver(g);
        auto result = solver.solve(0);
        benchmark::DoNotOptimize(result);
    }

    state.SetComplexityN(n);
    state.counters["nodes"] = n;
    state.counters["edges"] = g.m;
}

static void BM_LemonDijkstra_Sparse(benchmark::State& state) {
    int n = state.range(0);
    int m = n * 2;
    std::string key = "sparse_" + std::to_string(n) + "_" + std::to_string(m);
    auto& g = getOrCreateGraph(key, n, m, 42);

    for (auto _ : state) {
        auto result = DijkstraLemon::solve(g, 0);
        benchmark::DoNotOptimize(result);
    }

    state.SetComplexityN(n);
    state.counters["nodes"] = n;
    state.counters["edges"] = g.m;
}

// Register sparse benchmarks
BENCHMARK(BM_Dijkstra_Sparse)
    ->RangeMultiplier(2)
    ->Range(1000, 1000000)
    ->Complexity()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_NewSSSP_Sparse)
    ->RangeMultiplier(2)
    ->Range(1000, 1000000)
    ->Complexity()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_LemonDijkstra_Sparse)
    ->RangeMultiplier(2)
    ->Range(1000, 1000000)
    ->Complexity()
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Very Sparse Graphs (m = O(n)) - Best case for the new algorithm
// ============================================================================

static void BM_Dijkstra_VerySparse(benchmark::State& state) {
    int n = state.range(0);
    int m = n + n / 10;  // Very sparse
    std::string key = "vsparse_" + std::to_string(n);
    auto& g = getOrCreateGraph(key, n, m, 43);

    for (auto _ : state) {
        auto result = SimpleDijkstra::solve(g, 0);
        benchmark::DoNotOptimize(result);
    }

    state.counters["edges"] = g.m;
}

static void BM_NewSSSP_VerySparse(benchmark::State& state) {
    int n = state.range(0);
    int m = n + n / 10;
    std::string key = "vsparse_" + std::to_string(n);
    auto& g = getOrCreateGraph(key, n, m, 43);

    for (auto _ : state) {
        NewSSSP solver(g);
        auto result = solver.solve(0);
        benchmark::DoNotOptimize(result);
    }

    state.counters["edges"] = g.m;
}

BENCHMARK(BM_Dijkstra_VerySparse)
    ->RangeMultiplier(2)
    ->Range(10000, 1000000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_NewSSSP_VerySparse)
    ->RangeMultiplier(2)
    ->Range(10000, 1000000)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Dense Graphs (m = O(n^1.5)) - Dijkstra should be better here
// ============================================================================

static void BM_Dijkstra_Dense(benchmark::State& state) {
    int n = state.range(0);
    int m = static_cast<int>(std::pow(n, 1.5));
    std::string key = "dense_" + std::to_string(n);
    auto& g = getOrCreateGraph(key, n, m, 44);

    for (auto _ : state) {
        auto result = SimpleDijkstra::solve(g, 0);
        benchmark::DoNotOptimize(result);
    }

    state.counters["edges"] = g.m;
}

static void BM_NewSSSP_Dense(benchmark::State& state) {
    int n = state.range(0);
    int m = static_cast<int>(std::pow(n, 1.5));
    std::string key = "dense_" + std::to_string(n);
    auto& g = getOrCreateGraph(key, n, m, 44);

    for (auto _ : state) {
        NewSSSP solver(g);
        auto result = solver.solve(0);
        benchmark::DoNotOptimize(result);
    }

    state.counters["edges"] = g.m;
}

BENCHMARK(BM_Dijkstra_Dense)
    ->RangeMultiplier(2)
    ->Range(1000, 100000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_NewSSSP_Dense)
    ->RangeMultiplier(2)
    ->Range(1000, 100000)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Grid Graphs - Structured graphs with predictable shortest paths
// ============================================================================

static void BM_Dijkstra_Grid(benchmark::State& state) {
    int size = state.range(0);
    std::string key = "grid_" + std::to_string(size);
    auto& g = getOrCreateGrid(key, size, size, 45);

    for (auto _ : state) {
        auto result = SimpleDijkstra::solve(g, 0);
        benchmark::DoNotOptimize(result);
    }

    state.counters["nodes"] = size * size;
    state.counters["edges"] = g.m;
}

static void BM_NewSSSP_Grid(benchmark::State& state) {
    int size = state.range(0);
    std::string key = "grid_" + std::to_string(size);
    auto& g = getOrCreateGrid(key, size, size, 45);

    for (auto _ : state) {
        NewSSSP solver(g);
        auto result = solver.solve(0);
        benchmark::DoNotOptimize(result);
    }

    state.counters["nodes"] = size * size;
    state.counters["edges"] = g.m;
}

BENCHMARK(BM_Dijkstra_Grid)
    ->RangeMultiplier(2)
    ->Range(100, 1000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_NewSSSP_Grid)
    ->RangeMultiplier(2)
    ->Range(100, 1000)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Scale-Free Graphs - Real-world network structure
// ============================================================================

static void BM_Dijkstra_ScaleFree(benchmark::State& state) {
    int n = state.range(0);
    std::string key = "scalefree_" + std::to_string(n);
    auto& g = getOrCreateScaleFree(key, n, 46);

    for (auto _ : state) {
        auto result = SimpleDijkstra::solve(g, 0);
        benchmark::DoNotOptimize(result);
    }

    state.counters["edges"] = g.m;
}

static void BM_NewSSSP_ScaleFree(benchmark::State& state) {
    int n = state.range(0);
    std::string key = "scalefree_" + std::to_string(n);
    auto& g = getOrCreateScaleFree(key, n, 46);

    for (auto _ : state) {
        NewSSSP solver(g);
        auto result = solver.solve(0);
        benchmark::DoNotOptimize(result);
    }

    state.counters["edges"] = g.m;
}

BENCHMARK(BM_Dijkstra_ScaleFree)
    ->RangeMultiplier(2)
    ->Range(10000, 500000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_NewSSSP_ScaleFree)
    ->RangeMultiplier(2)
    ->Range(10000, 500000)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Huge Sparse Graphs - Testing scalability limits
// ============================================================================

static void BM_Dijkstra_Huge(benchmark::State& state) {
    int n = state.range(0);
    int m = n * 3;
    std::string key = "huge_" + std::to_string(n);
    auto& g = getOrCreateGraph(key, n, m, 47);

    for (auto _ : state) {
        auto result = SimpleDijkstra::solve(g, 0);
        benchmark::DoNotOptimize(result);
    }

    state.counters["nodes"] = n;
    state.counters["edges"] = g.m;
}

static void BM_NewSSSP_Huge(benchmark::State& state) {
    int n = state.range(0);
    int m = n * 3;
    std::string key = "huge_" + std::to_string(n);
    auto& g = getOrCreateGraph(key, n, m, 47);

    for (auto _ : state) {
        NewSSSP solver(g);
        auto result = solver.solve(0);
        benchmark::DoNotOptimize(result);
    }

    state.counters["nodes"] = n;
    state.counters["edges"] = g.m;
}

BENCHMARK(BM_Dijkstra_Huge)
    ->Arg(1000000)
    ->Arg(2000000)
    ->Arg(5000000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_NewSSSP_Huge)
    ->Arg(1000000)
    ->Arg(2000000)
    ->Arg(5000000)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Comparison at specific sizes for direct comparison
// ============================================================================

static void BM_Comparison_10K(benchmark::State& state) {
    int n = 10000;
    int m = 20000;
    std::string key = "comp_10k";
    auto& g = getOrCreateGraph(key, n, m, 100);

    bool use_new = state.range(0) == 1;

    for (auto _ : state) {
        if (use_new) {
            NewSSSP solver(g);
            auto result = solver.solve(0);
            benchmark::DoNotOptimize(result);
        } else {
            auto result = SimpleDijkstra::solve(g, 0);
            benchmark::DoNotOptimize(result);
        }
    }

    state.SetLabel(use_new ? "NewSSSP" : "Dijkstra");
}

BENCHMARK(BM_Comparison_10K)
    ->Arg(0)  // Dijkstra
    ->Arg(1)  // NewSSSP
    ->Unit(benchmark::kMillisecond);

static void BM_Comparison_100K(benchmark::State& state) {
    int n = 100000;
    int m = 200000;
    std::string key = "comp_100k";
    auto& g = getOrCreateGraph(key, n, m, 101);

    bool use_new = state.range(0) == 1;

    for (auto _ : state) {
        if (use_new) {
            NewSSSP solver(g);
            auto result = solver.solve(0);
            benchmark::DoNotOptimize(result);
        } else {
            auto result = SimpleDijkstra::solve(g, 0);
            benchmark::DoNotOptimize(result);
        }
    }

    state.SetLabel(use_new ? "NewSSSP" : "Dijkstra");
}

BENCHMARK(BM_Comparison_100K)
    ->Arg(0)
    ->Arg(1)
    ->Unit(benchmark::kMillisecond);

static void BM_Comparison_1M(benchmark::State& state) {
    int n = 1000000;
    int m = 2000000;
    std::string key = "comp_1m";
    auto& g = getOrCreateGraph(key, n, m, 102);

    bool use_new = state.range(0) == 1;

    for (auto _ : state) {
        if (use_new) {
            NewSSSP solver(g);
            auto result = solver.solve(0);
            benchmark::DoNotOptimize(result);
        } else {
            auto result = SimpleDijkstra::solve(g, 0);
            benchmark::DoNotOptimize(result);
        }
    }

    state.SetLabel(use_new ? "NewSSSP" : "Dijkstra");
}

BENCHMARK(BM_Comparison_1M)
    ->Arg(0)
    ->Arg(1)
    ->Unit(benchmark::kMillisecond);

// Main function
BENCHMARK_MAIN();

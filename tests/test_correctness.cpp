#include <gtest/gtest.h>
#include "new_sssp.hpp"
#include "dijkstra_lemon.hpp"
#include "graph_generator.hpp"
#include <cmath>

using namespace sssp;

/**
 * Correctness tests that compare the new algorithm with Dijkstra
 */

class CorrectnessTest : public ::testing::Test {
protected:
    static constexpr double EPSILON = 1e-9;

    void compareResults(const SimpleGraph& g, int source) {
        auto dijkstra_result = SimpleDijkstra::solve(g, source);

        NewSSSP solver(g);
        auto new_result = solver.solve(source);

        for (int i = 0; i < g.n; ++i) {
            if (dijkstra_result.distances[i] < INF) {
                EXPECT_NEAR(dijkstra_result.distances[i], new_result.distances[i], EPSILON)
                    << "Distance mismatch at node " << i
                    << " (Dijkstra: " << dijkstra_result.distances[i]
                    << ", New: " << new_result.distances[i] << ")";
            }
        }
    }
};

TEST_F(CorrectnessTest, SmallRandomGraphs) {
    for (int seed = 0; seed < 10; ++seed) {
        auto g = GraphGenerator::randomSparse(30, 100, 1.0, 100.0, seed);
        compareResults(g, 0);
    }
}

TEST_F(CorrectnessTest, MediumRandomGraphs) {
    for (int seed = 0; seed < 5; ++seed) {
        auto g = GraphGenerator::randomSparse(100, 500, 1.0, 100.0, seed + 100);
        compareResults(g, 0);
    }
}

TEST_F(CorrectnessTest, GridGraphs) {
    for (int size = 3; size <= 10; size += 2) {
        auto g = GraphGenerator::grid(size, size, 1.0, 10.0, size);
        compareResults(g, 0);
    }
}

TEST_F(CorrectnessTest, CompleteGraphs) {
    for (int n = 5; n <= 20; n += 5) {
        auto g = GraphGenerator::complete(n, 1.0, 100.0, n);
        compareResults(g, 0);
    }
}

TEST_F(CorrectnessTest, ScaleFreeGraphs) {
    for (int seed = 0; seed < 5; ++seed) {
        auto g = GraphGenerator::scaleFree(50, 3, 2, 1.0, 100.0, seed + 200);
        compareResults(g, 0);
    }
}

TEST_F(CorrectnessTest, SparseGraphs) {
    // Very sparse: m ~ n
    for (int seed = 0; seed < 5; ++seed) {
        int n = 100;
        int m = n + 10;  // Just slightly more than a tree
        auto g = GraphGenerator::randomSparse(n, m, 1.0, 100.0, seed + 300);
        compareResults(g, 0);
    }
}

TEST_F(CorrectnessTest, DenseGraphs) {
    // Dense: m ~ n^2 / 4
    for (int seed = 0; seed < 3; ++seed) {
        int n = 30;
        int m = n * n / 4;
        auto g = GraphGenerator::randomSparse(n, m, 1.0, 100.0, seed + 400);
        compareResults(g, 0);
    }
}

TEST_F(CorrectnessTest, DifferentSources) {
    auto g = GraphGenerator::randomSparse(50, 200, 1.0, 100.0, 42);

    // Test from different source nodes
    for (int source : {0, 10, 25, 49}) {
        compareResults(g, source);
    }
}

TEST_F(CorrectnessTest, UniformWeights) {
    // Uniform weight edges - tests path length correctness
    int n = 50;
    int m = 200;
    SimpleGraph g(n);

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> node_dist(0, n - 1);

    // Create spanning tree with weight 1
    std::vector<int> perm(n);
    std::iota(perm.begin(), perm.end(), 0);
    std::shuffle(perm.begin(), perm.end(), rng);

    for (int i = 1; i < n; ++i) {
        std::uniform_int_distribution<int> parent_dist(0, i - 1);
        int parent = perm[parent_dist(rng)];
        int child = perm[i];
        g.add_edge(parent, child, 1.0);
    }

    // Add more edges with weight 1
    for (int i = n - 1; i < m; ++i) {
        int u = node_dist(rng);
        int v = node_dist(rng);
        if (u != v) {
            g.add_edge(u, v, 1.0);
        }
    }

    compareResults(g, 0);
}

TEST_F(CorrectnessTest, VerySmallWeights) {
    auto g = GraphGenerator::randomSparse(30, 100, 0.001, 0.01, 42);
    compareResults(g, 0);
}

TEST_F(CorrectnessTest, VeryLargeWeights) {
    auto g = GraphGenerator::randomSparse(30, 100, 1e6, 1e9, 42);
    compareResults(g, 0);
}

TEST_F(CorrectnessTest, MixedWeightRanges) {
    int n = 50;
    SimpleGraph g(n);

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> node_dist(0, n - 1);

    // Spanning tree
    for (int i = 1; i < n; ++i) {
        std::uniform_int_distribution<int> parent_dist(0, i - 1);
        g.add_edge(parent_dist(rng), i, 1.0);
    }

    // Add edges with varying weights
    std::vector<double> weight_choices = {0.001, 0.1, 1.0, 10.0, 100.0, 1000.0};
    std::uniform_int_distribution<size_t> weight_idx(0, weight_choices.size() - 1);

    for (int i = 0; i < 100; ++i) {
        int u = node_dist(rng);
        int v = node_dist(rng);
        if (u != v) {
            g.add_edge(u, v, weight_choices[weight_idx(rng)]);
        }
    }

    compareResults(g, 0);
}

// Test with LEMON Dijkstra as well
TEST_F(CorrectnessTest, CompareWithLemonDijkstra) {
    for (int seed = 0; seed < 5; ++seed) {
        auto g = GraphGenerator::randomSparse(100, 500, 1.0, 100.0, seed);

        auto lemon_result = DijkstraLemon::solve(g, 0);

        NewSSSP solver(g);
        auto new_result = solver.solve(0);

        for (int i = 0; i < g.n; ++i) {
            if (lemon_result.distances[i] < INF) {
                EXPECT_NEAR(lemon_result.distances[i], new_result.distances[i], EPSILON)
                    << "Distance mismatch with LEMON at node " << i
                    << " (seed=" << seed << ")";
            }
        }
    }
}

// Stress test with larger graphs
TEST_F(CorrectnessTest, LargerGraphStress) {
    // This test takes longer but ensures correctness on bigger instances
    for (int seed = 0; seed < 3; ++seed) {
        auto g = GraphGenerator::randomSparse(500, 2500, 1.0, 100.0, seed + 500);
        compareResults(g, 0);
    }
}

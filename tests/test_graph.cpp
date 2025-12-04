#include <gtest/gtest.h>
#include "graph_types.hpp"
#include "graph_generator.hpp"

using namespace sssp;

TEST(GraphTypesTest, SimpleGraphCreation) {
    SimpleGraph g(5);
    EXPECT_EQ(g.n, 5);
    EXPECT_EQ(g.m, 0);

    g.add_edge(0, 1, 1.0);
    g.add_edge(0, 2, 2.0);
    EXPECT_EQ(g.m, 2);
    EXPECT_EQ(g.adj[0].size(), 2);
}

TEST(GraphTypesTest, SimpleGraphAdjacency) {
    SimpleGraph g(3);
    g.add_edge(0, 1, 1.5);
    g.add_edge(0, 2, 2.5);
    g.add_edge(1, 2, 3.5);

    EXPECT_EQ(g.adj[0].size(), 2);
    EXPECT_EQ(g.adj[1].size(), 1);
    EXPECT_EQ(g.adj[2].size(), 0);

    // Check specific edges
    bool found_01 = false, found_02 = false;
    for (const auto& [v, w] : g.adj[0]) {
        if (v == 1 && std::abs(w - 1.5) < 1e-9) found_01 = true;
        if (v == 2 && std::abs(w - 2.5) < 1e-9) found_02 = true;
    }
    EXPECT_TRUE(found_01);
    EXPECT_TRUE(found_02);
}

TEST(GraphGeneratorTest, RandomSparseGraph) {
    int n = 100;
    int m = 500;
    auto g = GraphGenerator::randomSparse(n, m);

    EXPECT_EQ(g.n, n);
    EXPECT_GE(g.m, n - 1);  // At least spanning tree edges
    EXPECT_LE(g.m, m);      // At most m edges

    // Check all weights are positive
    for (int u = 0; u < n; ++u) {
        for (const auto& [v, w] : g.adj[u]) {
            EXPECT_GT(w, 0);
            EXPECT_GE(v, 0);
            EXPECT_LT(v, n);
        }
    }
}

TEST(GraphGeneratorTest, GridGraph) {
    int rows = 10, cols = 10;
    auto g = GraphGenerator::grid(rows, cols);

    EXPECT_EQ(g.n, rows * cols);

    // Corner nodes should have 2 outgoing edges
    // Edge nodes should have 3 outgoing edges
    // Interior nodes should have 4 outgoing edges

    // Check node 0 (corner)
    EXPECT_EQ(g.adj[0].size(), 2);  // right and down

    // Check center node
    int center = (rows / 2) * cols + (cols / 2);
    EXPECT_EQ(g.adj[center].size(), 4);  // all 4 directions
}

TEST(GraphGeneratorTest, CompleteGraph) {
    int n = 10;
    auto g = GraphGenerator::complete(n);

    EXPECT_EQ(g.n, n);
    EXPECT_EQ(g.m, n * (n - 1));

    // Each node should have n-1 outgoing edges
    for (int u = 0; u < n; ++u) {
        EXPECT_EQ(g.adj[u].size(), static_cast<size_t>(n - 1));
    }
}

TEST(GraphGeneratorTest, ScaleFreeGraph) {
    int n = 100;
    auto g = GraphGenerator::scaleFree(n, 3, 2);

    EXPECT_EQ(g.n, n);
    EXPECT_GT(g.m, 0);

    // Check that graph is connected (at least reachable from node 0)
    std::vector<bool> visited(n, false);
    std::queue<int> q;
    q.push(0);
    visited[0] = true;
    int count = 1;

    while (!q.empty()) {
        int u = q.front();
        q.pop();
        for (const auto& [v, w] : g.adj[u]) {
            if (!visited[v]) {
                visited[v] = true;
                q.push(v);
                count++;
            }
        }
    }

    // Most nodes should be reachable in a scale-free graph
    EXPECT_GT(count, n / 2);
}

TEST(GraphGeneratorTest, Determinism) {
    // Same seed should produce same graph
    auto g1 = GraphGenerator::randomSparse(50, 200, 1.0, 100.0, 12345);
    auto g2 = GraphGenerator::randomSparse(50, 200, 1.0, 100.0, 12345);

    EXPECT_EQ(g1.n, g2.n);
    EXPECT_EQ(g1.m, g2.m);

    for (int u = 0; u < g1.n; ++u) {
        ASSERT_EQ(g1.adj[u].size(), g2.adj[u].size());
        for (size_t i = 0; i < g1.adj[u].size(); ++i) {
            EXPECT_EQ(g1.adj[u][i].first, g2.adj[u][i].first);
            EXPECT_DOUBLE_EQ(g1.adj[u][i].second, g2.adj[u][i].second);
        }
    }
}

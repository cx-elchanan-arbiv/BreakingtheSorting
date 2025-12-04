#include <gtest/gtest.h>
#include "dijkstra_lemon.hpp"
#include "graph_generator.hpp"

using namespace sssp;

TEST(DijkstraTest, SimplePathGraph) {
    // Path: 0 -> 1 -> 2 -> 3
    SimpleGraph g(4);
    g.add_edge(0, 1, 1.0);
    g.add_edge(1, 2, 2.0);
    g.add_edge(2, 3, 3.0);

    auto result = SimpleDijkstra::solve(g, 0);

    EXPECT_DOUBLE_EQ(result.distances[0], 0.0);
    EXPECT_DOUBLE_EQ(result.distances[1], 1.0);
    EXPECT_DOUBLE_EQ(result.distances[2], 3.0);
    EXPECT_DOUBLE_EQ(result.distances[3], 6.0);

    EXPECT_EQ(result.predecessors[0], -1);
    EXPECT_EQ(result.predecessors[1], 0);
    EXPECT_EQ(result.predecessors[2], 1);
    EXPECT_EQ(result.predecessors[3], 2);
}

TEST(DijkstraTest, DiamondGraph) {
    // Diamond: 0 -> 1, 0 -> 2, 1 -> 3, 2 -> 3
    //   1
    //  / \
    // 0   3
    //  \ /
    //   2
    SimpleGraph g(4);
    g.add_edge(0, 1, 1.0);
    g.add_edge(0, 2, 3.0);
    g.add_edge(1, 3, 4.0);
    g.add_edge(2, 3, 1.0);

    auto result = SimpleDijkstra::solve(g, 0);

    EXPECT_DOUBLE_EQ(result.distances[0], 0.0);
    EXPECT_DOUBLE_EQ(result.distances[1], 1.0);
    EXPECT_DOUBLE_EQ(result.distances[2], 3.0);
    EXPECT_DOUBLE_EQ(result.distances[3], 4.0);  // 0->2->3 = 3+1 = 4, better than 0->1->3 = 1+4 = 5

    // Predecessor of 3 should be 2 (shorter path)
    EXPECT_EQ(result.predecessors[3], 2);
}

TEST(DijkstraTest, DisconnectedGraph) {
    SimpleGraph g(4);
    g.add_edge(0, 1, 1.0);
    // Nodes 2 and 3 are not reachable from 0

    auto result = SimpleDijkstra::solve(g, 0);

    EXPECT_DOUBLE_EQ(result.distances[0], 0.0);
    EXPECT_DOUBLE_EQ(result.distances[1], 1.0);
    EXPECT_EQ(result.distances[2], INF);
    EXPECT_EQ(result.distances[3], INF);
}

TEST(DijkstraTest, SingleNode) {
    SimpleGraph g(1);
    auto result = SimpleDijkstra::solve(g, 0);

    EXPECT_DOUBLE_EQ(result.distances[0], 0.0);
    EXPECT_EQ(result.predecessors[0], -1);
}

TEST(DijkstraLemonTest, CompareWithSimple) {
    // Generate random graph and compare LEMON Dijkstra with simple implementation
    auto g = GraphGenerator::randomSparse(50, 200, 1.0, 100.0, 42);

    auto simple_result = SimpleDijkstra::solve(g, 0);
    auto lemon_result = DijkstraLemon::solve(g, 0);

    // Both should give the same distances
    for (int i = 0; i < g.n; ++i) {
        if (simple_result.distances[i] < INF) {
            EXPECT_NEAR(simple_result.distances[i], lemon_result.distances[i], 1e-9)
                << "Distance mismatch at node " << i;
        }
    }
}

TEST(DijkstraTest, GridGraph) {
    auto g = GraphGenerator::grid(5, 5, 1.0, 1.0, 42);  // Uniform weights

    auto result = SimpleDijkstra::solve(g, 0);

    // Node 0 is at (0,0), node 24 is at (4,4)
    // Shortest path in a grid with uniform weights
    EXPECT_DOUBLE_EQ(result.distances[0], 0.0);
    EXPECT_GT(result.distances[24], 0.0);  // Should be reachable
    EXPECT_LT(result.distances[24], INF);
}

TEST(DijkstraTest, LargeRandomGraph) {
    // Test on larger graph
    auto g = GraphGenerator::randomSparse(1000, 5000, 1.0, 100.0, 123);

    auto result = SimpleDijkstra::solve(g, 0);

    // Source should have distance 0
    EXPECT_DOUBLE_EQ(result.distances[0], 0.0);

    // Count reachable nodes
    int reachable = 0;
    for (int i = 0; i < g.n; ++i) {
        if (result.distances[i] < INF) {
            reachable++;
        }
    }

    // Most nodes should be reachable in a connected random graph
    EXPECT_GT(reachable, g.n / 2);
}

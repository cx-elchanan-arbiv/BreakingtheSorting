#include <gtest/gtest.h>
#include "new_sssp.hpp"
#include "graph_generator.hpp"
#include "block_data_structure.hpp"

using namespace sssp;

// Tests for BlockDataStructure
TEST(BlockDataStructureTest, BasicOperations) {
    BlockDataStructure ds;
    ds.initialize(3, 1000.0, 10);

    ds.insert(0, 5.0);
    ds.insert(1, 3.0);
    ds.insert(2, 7.0);

    EXPECT_EQ(ds.size(), 3);
    EXPECT_FALSE(ds.empty());
}

TEST(BlockDataStructureTest, PullOperation) {
    BlockDataStructure ds;
    ds.initialize(2, 1000.0, 10);

    ds.insert(0, 5.0);
    ds.insert(1, 3.0);
    ds.insert(2, 7.0);
    ds.insert(3, 1.0);

    auto [keys, bound] = ds.pull();

    // Should get the 2 smallest
    EXPECT_LE(keys.size(), 2);
    EXPECT_EQ(ds.size(), 4 - keys.size());
}

TEST(BlockDataStructureTest, BatchPrepend) {
    BlockDataStructure ds;
    ds.initialize(3, 1000.0, 20);

    ds.insert(5, 50.0);
    ds.insert(6, 60.0);

    std::vector<BlockDataStructure::KeyValue> items = {
        {0, 5.0}, {1, 3.0}, {2, 7.0}
    };
    ds.batchPrepend(items);

    EXPECT_EQ(ds.size(), 5);

    // Pull should get smallest elements
    auto [keys, bound] = ds.pull();
    EXPECT_LE(keys.size(), 3);
}

TEST(BlockDataStructureTest, DuplicateKeys) {
    BlockDataStructure ds;
    ds.initialize(3, 1000.0, 10);

    ds.insert(0, 10.0);
    ds.insert(0, 5.0);  // Should update to smaller value

    EXPECT_EQ(ds.size(), 1);
    EXPECT_DOUBLE_EQ(ds.getValue(0), 5.0);
}

// Tests for NewSSSP algorithm
TEST(NewSSSPTest, SimplePathGraph) {
    SimpleGraph g(4);
    g.add_edge(0, 1, 1.0);
    g.add_edge(1, 2, 2.0);
    g.add_edge(2, 3, 3.0);

    NewSSSP solver(g);
    auto result = solver.solve(0);

    EXPECT_DOUBLE_EQ(result.distances[0], 0.0);
    EXPECT_DOUBLE_EQ(result.distances[1], 1.0);
    EXPECT_DOUBLE_EQ(result.distances[2], 3.0);
    EXPECT_DOUBLE_EQ(result.distances[3], 6.0);
}

TEST(NewSSSPTest, DiamondGraph) {
    SimpleGraph g(4);
    g.add_edge(0, 1, 1.0);
    g.add_edge(0, 2, 3.0);
    g.add_edge(1, 3, 4.0);
    g.add_edge(2, 3, 1.0);

    NewSSSP solver(g);
    auto result = solver.solve(0);

    EXPECT_DOUBLE_EQ(result.distances[0], 0.0);
    EXPECT_DOUBLE_EQ(result.distances[1], 1.0);
    EXPECT_DOUBLE_EQ(result.distances[2], 3.0);
    EXPECT_DOUBLE_EQ(result.distances[3], 4.0);
}

TEST(NewSSSPTest, SingleNode) {
    SimpleGraph g(1);

    NewSSSP solver(g);
    auto result = solver.solve(0);

    EXPECT_DOUBLE_EQ(result.distances[0], 0.0);
}

TEST(NewSSSPTest, TwoNodes) {
    SimpleGraph g(2);
    g.add_edge(0, 1, 5.0);

    NewSSSP solver(g);
    auto result = solver.solve(0);

    EXPECT_DOUBLE_EQ(result.distances[0], 0.0);
    EXPECT_DOUBLE_EQ(result.distances[1], 5.0);
}

TEST(NewSSSPTest, StarGraph) {
    // Star graph: 0 is center, connected to all others
    int n = 10;
    SimpleGraph g(n);
    for (int i = 1; i < n; ++i) {
        g.add_edge(0, i, static_cast<double>(i));
    }

    NewSSSP solver(g);
    auto result = solver.solve(0);

    EXPECT_DOUBLE_EQ(result.distances[0], 0.0);
    for (int i = 1; i < n; ++i) {
        EXPECT_DOUBLE_EQ(result.distances[i], static_cast<double>(i));
    }
}

TEST(NewSSSPTest, SmallRandomGraph) {
    auto g = GraphGenerator::randomSparse(20, 50, 1.0, 10.0, 42);

    NewSSSP solver(g);
    auto result = solver.solve(0);

    // Source should have distance 0
    EXPECT_DOUBLE_EQ(result.distances[0], 0.0);

    // All reachable distances should be non-negative
    for (int i = 0; i < g.n; ++i) {
        EXPECT_GE(result.distances[i], 0.0);
    }
}

TEST(NewSSSPTest, GridGraph) {
    auto g = GraphGenerator::grid(4, 4, 1.0, 2.0, 42);

    NewSSSP solver(g);
    auto result = solver.solve(0);

    EXPECT_DOUBLE_EQ(result.distances[0], 0.0);

    // All nodes should be reachable in a grid
    for (int i = 0; i < g.n; ++i) {
        EXPECT_LT(result.distances[i], INF);
    }
}

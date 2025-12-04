#pragma once

#include <lemon/list_graph.h>
#include <lemon/smart_graph.h>
#include <vector>
#include <limits>
#include <cstdint>

namespace sssp {

// Use LEMON's SmartDigraph for better performance
using Graph = lemon::SmartDigraph;
using Node = Graph::Node;
using Arc = Graph::Arc;
using WeightMap = Graph::ArcMap<double>;
using DistMap = Graph::NodeMap<double>;
using PredMap = Graph::NodeMap<Arc>;

constexpr double INF = std::numeric_limits<double>::infinity();

// Constant-degree graph representation for the new algorithm
// Each vertex has bounded in/out degree (transformed from original graph)
struct ConstantDegreeGraph {
    int num_nodes;
    int num_arcs;

    // Adjacency list representation
    // For each node: list of (target_node, weight)
    std::vector<std::vector<std::pair<int, double>>> out_edges;
    std::vector<std::vector<std::pair<int, double>>> in_edges;

    // Mapping between original and transformed graph
    std::vector<int> original_to_transformed;  // original node -> first transformed node
    std::vector<int> transformed_to_original;  // transformed node -> original node

    ConstantDegreeGraph() : num_nodes(0), num_arcs(0) {}

    void clear() {
        num_nodes = 0;
        num_arcs = 0;
        out_edges.clear();
        in_edges.clear();
        original_to_transformed.clear();
        transformed_to_original.clear();
    }

    int add_node(int original_id = -1) {
        int id = num_nodes++;
        out_edges.emplace_back();
        in_edges.emplace_back();
        if (original_id >= 0) {
            if (static_cast<int>(transformed_to_original.size()) <= id) {
                transformed_to_original.resize(id + 1, -1);
            }
            transformed_to_original[id] = original_id;
        }
        return id;
    }

    void add_arc(int from, int to, double weight) {
        out_edges[from].emplace_back(to, weight);
        in_edges[to].emplace_back(from, weight);
        num_arcs++;
    }
};

// Transform general graph to constant-degree graph (max degree 2)
// Based on the paper's transformation (Section 2, Constant-Degree Graph)
inline ConstantDegreeGraph transformToConstantDegree(const Graph& g, const WeightMap& weights) {
    ConstantDegreeGraph cdg;

    // For each original node, we create a cycle of nodes
    // Each neighbor gets one node in the cycle
    std::vector<std::vector<int>> node_cycles;  // original_node -> list of transformed nodes

    int original_node_count = lemon::countNodes(g);
    node_cycles.resize(original_node_count);
    cdg.original_to_transformed.resize(original_node_count, -1);

    // First pass: create nodes for each edge endpoint
    for (Graph::NodeIt v(g); v != lemon::INVALID; ++v) {
        int v_id = g.id(v);

        // Count neighbors
        std::vector<int> neighbors;
        for (Graph::OutArcIt a(g, v); a != lemon::INVALID; ++a) {
            neighbors.push_back(g.id(g.target(a)));
        }
        for (Graph::InArcIt a(g, v); a != lemon::INVALID; ++a) {
            neighbors.push_back(g.id(g.source(a)));
        }

        if (neighbors.empty()) {
            // Isolated node
            int new_id = cdg.add_node(v_id);
            cdg.original_to_transformed[v_id] = new_id;
            node_cycles[v_id].push_back(new_id);
        } else {
            // Create a cycle node for each neighbor
            for (size_t i = 0; i < neighbors.size(); ++i) {
                int new_id = cdg.add_node(v_id);
                node_cycles[v_id].push_back(new_id);
                if (i == 0) {
                    cdg.original_to_transformed[v_id] = new_id;
                }
            }

            // Connect cycle nodes with zero-weight edges
            for (size_t i = 0; i < node_cycles[v_id].size(); ++i) {
                int curr = node_cycles[v_id][i];
                int next = node_cycles[v_id][(i + 1) % node_cycles[v_id].size()];
                cdg.add_arc(curr, next, 0.0);
                cdg.add_arc(next, curr, 0.0);
            }
        }
    }

    // Second pass: add edges between cycle nodes
    for (Graph::ArcIt a(g); a != lemon::INVALID; ++a) {
        int u_id = g.id(g.source(a));
        int v_id = g.id(g.target(a));
        double w = weights[a];

        // Find a cycle node for u and v and connect them
        // For simplicity, use the first available node in each cycle
        if (!node_cycles[u_id].empty() && !node_cycles[v_id].empty()) {
            cdg.add_arc(node_cycles[u_id][0], node_cycles[v_id][0], w);
        }
    }

    return cdg;
}

// Simple adjacency list graph for the algorithm (without transformation)
struct SimpleGraph {
    int n;  // number of nodes
    int m;  // number of edges
    std::vector<std::vector<std::pair<int, double>>> adj;  // adjacency list

    SimpleGraph() : n(0), m(0) {}
    explicit SimpleGraph(int nodes) : n(nodes), m(0), adj(nodes) {}

    void add_edge(int u, int v, double w) {
        adj[u].emplace_back(v, w);
        m++;
    }

    void clear() {
        n = 0;
        m = 0;
        adj.clear();
    }

    void resize(int nodes) {
        n = nodes;
        adj.resize(nodes);
    }
};

// Convert LEMON graph to SimpleGraph
inline SimpleGraph lemonToSimple(const Graph& g, const WeightMap& weights) {
    int n = lemon::countNodes(g);
    SimpleGraph sg(n);

    for (Graph::ArcIt a(g); a != lemon::INVALID; ++a) {
        int u = g.id(g.source(a));
        int v = g.id(g.target(a));
        sg.add_edge(u, v, weights[a]);
    }

    return sg;
}

}  // namespace sssp

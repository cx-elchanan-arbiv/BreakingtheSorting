#pragma once

#include "graph_types.hpp"
#include <lemon/dijkstra.h>
#include <vector>

namespace sssp {

/**
 * Wrapper around LEMON's Dijkstra implementation
 */
class DijkstraLemon {
public:
    struct Result {
        std::vector<double> distances;
        std::vector<int> predecessors;
        int source;
    };

    static Result solve(const Graph& graph, const WeightMap& weights, Node source) {
        int n = lemon::countNodes(graph);
        Result result;
        result.distances.resize(n, INF);
        result.predecessors.resize(n, -1);
        result.source = graph.id(source);

        // Use LEMON's Dijkstra
        lemon::Dijkstra<Graph, WeightMap> dijkstra(graph, weights);
        dijkstra.run(source);

        // Extract results
        for (Graph::NodeIt v(graph); v != lemon::INVALID; ++v) {
            int v_id = graph.id(v);
            if (dijkstra.reached(v)) {
                result.distances[v_id] = dijkstra.dist(v);
                if (dijkstra.predArc(v) != lemon::INVALID) {
                    result.predecessors[v_id] = graph.id(graph.source(dijkstra.predArc(v)));
                }
            }
        }

        return result;
    }

    // Solve using SimpleGraph format
    static Result solve(const SimpleGraph& graph, int source) {
        // Convert SimpleGraph to LEMON graph
        Graph lemon_graph;
        WeightMap weights(lemon_graph);

        std::vector<Node> nodes;
        nodes.reserve(graph.n);

        for (int i = 0; i < graph.n; ++i) {
            nodes.push_back(lemon_graph.addNode());
        }

        for (int u = 0; u < graph.n; ++u) {
            for (const auto& [v, w] : graph.adj[u]) {
                Arc arc = lemon_graph.addArc(nodes[u], nodes[v]);
                weights[arc] = w;
            }
        }

        return solve(lemon_graph, weights, nodes[source]);
    }
};

/**
 * Simple Dijkstra implementation for comparison (without LEMON)
 * Uses std::priority_queue with Fibonacci-heap-like behavior
 */
class SimpleDijkstra {
public:
    struct Result {
        std::vector<double> distances;
        std::vector<int> predecessors;
        int source;
    };

    static Result solve(const SimpleGraph& graph, int source) {
        int n = graph.n;
        Result result;
        result.distances.assign(n, INF);
        result.predecessors.assign(n, -1);
        result.source = source;

        // Priority queue: (distance, vertex)
        using PQEntry = std::pair<double, int>;
        std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<PQEntry>> pq;

        result.distances[source] = 0.0;
        pq.push({0.0, source});

        while (!pq.empty()) {
            auto [dist, u] = pq.top();
            pq.pop();

            if (dist > result.distances[u]) {
                continue;  // Outdated entry
            }

            for (const auto& [v, w] : graph.adj[u]) {
                double new_dist = result.distances[u] + w;
                if (new_dist < result.distances[v]) {
                    result.distances[v] = new_dist;
                    result.predecessors[v] = u;
                    pq.push({new_dist, v});
                }
            }
        }

        return result;
    }
};

}  // namespace sssp

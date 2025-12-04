#pragma once

#include "graph_types.hpp"
#include <random>
#include <set>
#include <algorithm>

namespace sssp {

/**
 * Graph generators for testing and benchmarking
 */
class GraphGenerator {
public:
    // Random sparse graph (Erdos-Renyi-like)
    static SimpleGraph randomSparse(int n, int m, double min_weight = 1.0,
                                    double max_weight = 100.0, unsigned seed = 42) {
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> node_dist(0, n - 1);
        std::uniform_real_distribution<double> weight_dist(min_weight, max_weight);

        SimpleGraph g(n);
        std::set<std::pair<int, int>> edges;

        // Ensure connectivity: create a spanning tree first
        std::vector<int> perm(n);
        std::iota(perm.begin(), perm.end(), 0);
        std::shuffle(perm.begin(), perm.end(), rng);

        for (int i = 1; i < n; ++i) {
            std::uniform_int_distribution<int> parent_dist(0, i - 1);
            int parent = perm[parent_dist(rng)];
            int child = perm[i];

            g.add_edge(parent, child, weight_dist(rng));
            edges.insert({parent, child});
        }

        // Add remaining random edges
        int remaining = m - (n - 1);
        int attempts = 0;
        while (remaining > 0 && attempts < m * 10) {
            int u = node_dist(rng);
            int v = node_dist(rng);
            if (u != v && edges.find({u, v}) == edges.end()) {
                g.add_edge(u, v, weight_dist(rng));
                edges.insert({u, v});
                remaining--;
            }
            attempts++;
        }

        return g;
    }

    // Grid graph (good for testing shortest paths)
    static SimpleGraph grid(int rows, int cols, double min_weight = 1.0,
                           double max_weight = 10.0, unsigned seed = 42) {
        std::mt19937 rng(seed);
        std::uniform_real_distribution<double> weight_dist(min_weight, max_weight);

        int n = rows * cols;
        SimpleGraph g(n);

        auto idx = [cols](int r, int c) { return r * cols + c; };

        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                // Right edge
                if (c + 1 < cols) {
                    g.add_edge(idx(r, c), idx(r, c + 1), weight_dist(rng));
                }
                // Down edge
                if (r + 1 < rows) {
                    g.add_edge(idx(r, c), idx(r + 1, c), weight_dist(rng));
                }
                // Left edge (for directed graph)
                if (c > 0) {
                    g.add_edge(idx(r, c), idx(r, c - 1), weight_dist(rng));
                }
                // Up edge (for directed graph)
                if (r > 0) {
                    g.add_edge(idx(r, c), idx(r - 1, c), weight_dist(rng));
                }
            }
        }

        return g;
    }

    // Complete graph (dense)
    static SimpleGraph complete(int n, double min_weight = 1.0,
                                double max_weight = 100.0, unsigned seed = 42) {
        std::mt19937 rng(seed);
        std::uniform_real_distribution<double> weight_dist(min_weight, max_weight);

        SimpleGraph g(n);
        for (int u = 0; u < n; ++u) {
            for (int v = 0; v < n; ++v) {
                if (u != v) {
                    g.add_edge(u, v, weight_dist(rng));
                }
            }
        }
        return g;
    }

    // Random graph with specific average degree
    static SimpleGraph randomWithDegree(int n, double avg_degree,
                                        double min_weight = 1.0, double max_weight = 100.0,
                                        unsigned seed = 42) {
        int m = static_cast<int>(n * avg_degree);
        return randomSparse(n, m, min_weight, max_weight, seed);
    }

    // Scale-free graph (Barabasi-Albert model) - common in real networks
    static SimpleGraph scaleFree(int n, int m0, int m_edges_per_node,
                                 double min_weight = 1.0, double max_weight = 100.0,
                                 unsigned seed = 42) {
        std::mt19937 rng(seed);
        std::uniform_real_distribution<double> weight_dist(min_weight, max_weight);
        std::uniform_real_distribution<double> prob_dist(0.0, 1.0);

        SimpleGraph g(n);

        // Start with m0 nodes fully connected
        for (int u = 0; u < m0 && u < n; ++u) {
            for (int v = u + 1; v < m0 && v < n; ++v) {
                g.add_edge(u, v, weight_dist(rng));
                g.add_edge(v, u, weight_dist(rng));
            }
        }

        std::vector<int> degrees(n, 0);
        for (int i = 0; i < m0; ++i) {
            degrees[i] = m0 - 1;
        }

        // Add remaining nodes with preferential attachment
        for (int new_node = m0; new_node < n; ++new_node) {
            std::set<int> targets;
            int total_degree = 0;
            for (int i = 0; i < new_node; ++i) {
                total_degree += degrees[i];
            }

            while (static_cast<int>(targets.size()) < m_edges_per_node &&
                   static_cast<int>(targets.size()) < new_node) {
                // Preferential attachment
                double r = prob_dist(rng) * total_degree;
                double cumsum = 0;
                for (int i = 0; i < new_node; ++i) {
                    cumsum += degrees[i];
                    if (cumsum >= r) {
                        targets.insert(i);
                        break;
                    }
                }
            }

            for (int target : targets) {
                g.add_edge(new_node, target, weight_dist(rng));
                g.add_edge(target, new_node, weight_dist(rng));
                degrees[new_node]++;
                degrees[target]++;
            }
        }

        return g;
    }

    // Generate LEMON graph from SimpleGraph
    static std::pair<Graph, WeightMap*> toLemon(const SimpleGraph& sg) {
        Graph g;
        auto* weights = new WeightMap(g);

        std::vector<Node> nodes;
        for (int i = 0; i < sg.n; ++i) {
            nodes.push_back(g.addNode());
        }

        for (int u = 0; u < sg.n; ++u) {
            for (const auto& [v, w] : sg.adj[u]) {
                Arc arc = g.addArc(nodes[u], nodes[v]);
                (*weights)[arc] = w;
            }
        }

        return {g, weights};
    }
};

}  // namespace sssp

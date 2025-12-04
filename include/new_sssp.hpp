#pragma once

#include "graph_types.hpp"
#include "block_data_structure.hpp"
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <cmath>
#include <algorithm>
#include <functional>

namespace sssp {

/**
 * Implementation of the O(m log^{2/3} n) SSSP algorithm from
 * "Breaking the Sorting Barrier for Directed Single-Source Shortest Paths"
 * by Duan, Mao, Mao, Shu, and Yin (2025)
 */
class NewSSSP {
public:
    // Result structure
    struct Result {
        std::vector<double> distances;
        std::vector<int> predecessors;
        int source;
    };

private:
    const SimpleGraph& graph;
    int n, m;
    int k, t;  // Parameters: k = log^{1/3}(n), t = log^{2/3}(n)
    int max_level;

    // Global state
    std::vector<double> d_hat;     // Distance estimates
    std::vector<int> pred;         // Predecessors
    std::vector<bool> complete;    // Whether vertex is complete

    // Statistics
    mutable size_t relaxation_count = 0;

public:
    explicit NewSSSP(const SimpleGraph& g)
        : graph(g), n(g.n), m(g.m) {
        // Set parameters
        if (n <= 1) {
            k = t = max_level = 1;
        } else {
            double log_n = std::log2(static_cast<double>(n));
            k = std::max(2, static_cast<int>(std::floor(std::pow(log_n, 1.0/3.0))));
            t = std::max(2, static_cast<int>(std::floor(std::pow(log_n, 2.0/3.0))));
            max_level = std::max(1, static_cast<int>(std::ceil(log_n / t)));
        }
    }

    Result solve(int source) {
        // Initialize
        d_hat.assign(n, INF);
        pred.assign(n, -1);
        complete.assign(n, false);
        relaxation_count = 0;

        d_hat[source] = 0.0;
        complete[source] = true;

        // Relax edges from source
        for (const auto& [v, w] : graph.adj[source]) {
            if (d_hat[source] + w < d_hat[v]) {
                d_hat[v] = d_hat[source] + w;
                pred[v] = source;
            }
        }

        // Call main algorithm
        std::set<int> S = {source};
        auto [B_prime, U] = BMSSP(max_level, INF, S);

        Result result;
        result.distances = d_hat;
        result.predecessors = pred;
        result.source = source;
        return result;
    }

    size_t getRelaxationCount() const { return relaxation_count; }

private:
    // Bounded Multi-Source Shortest Path (Algorithm 3)
    std::pair<double, std::set<int>> BMSSP(int level, double B, const std::set<int>& S) {
        if (level == 0) {
            return baseCase(B, S);
        }

        // FindPivots
        auto [P, W] = findPivots(B, S);

        if (P.empty()) {
            // All vertices complete
            return {B, W};
        }

        // Initialize data structure D
        int M = static_cast<int>(std::pow(2, (level - 1) * t));
        M = std::max(1, std::min(M, n));

        BlockDataStructure D;
        D.initialize(M, B, k * static_cast<int>(std::pow(2, level * t)));

        // Insert pivots into D
        for (int x : P) {
            if (d_hat[x] < B) {
                D.insert(x, d_hat[x]);
            }
        }

        // Initialize
        double B_prime_0 = INF;
        for (int x : P) {
            if (complete[x]) {
                B_prime_0 = std::min(B_prime_0, d_hat[x]);
            }
        }
        if (B_prime_0 == INF && !P.empty()) {
            B_prime_0 = d_hat[*P.begin()];
        }

        std::set<int> U;
        double B_prime_i = B_prime_0;
        int size_limit = k * static_cast<int>(std::pow(2, level * t));
        size_limit = std::max(1, std::min(size_limit, n));

        // Main loop
        while (static_cast<int>(U.size()) < size_limit && !D.empty()) {
            // Pull from D
            auto [Si_vec, Bi] = D.pull();
            std::set<int> Si(Si_vec.begin(), Si_vec.end());

            if (Si.empty()) break;

            // Recursive call
            auto [B_prime_i_new, Ui] = BMSSP(level - 1, Bi, Si);
            B_prime_i = B_prime_i_new;

            // Add Ui to U
            U.insert(Ui.begin(), Ui.end());

            // Relax edges from Ui
            std::vector<BlockDataStructure::KeyValue> K;
            for (int u : Ui) {
                for (const auto& [v, w] : graph.adj[u]) {
                    relaxation_count++;
                    if (d_hat[u] + w <= d_hat[v]) {
                        d_hat[v] = d_hat[u] + w;
                        pred[v] = u;

                        if (d_hat[u] + w >= Bi && d_hat[u] + w < B) {
                            D.insert(v, d_hat[u] + w);
                        } else if (d_hat[u] + w >= B_prime_i && d_hat[u] + w < Bi) {
                            K.emplace_back(v, d_hat[u] + w);
                        }
                    }
                }
            }

            // BatchPrepend K and vertices from Si that need to go back
            for (int x : Si) {
                if (d_hat[x] >= B_prime_i && d_hat[x] < Bi) {
                    K.emplace_back(x, d_hat[x]);
                }
            }
            D.batchPrepend(K);
        }

        // Final B'
        double B_prime = std::min(B_prime_i, B);

        // Add vertices from W with d_hat < B'
        for (int x : W) {
            if (d_hat[x] < B_prime) {
                U.insert(x);
            }
        }

        return {B_prime, U};
    }

    // Base case (Algorithm 2) - mini Dijkstra
    std::pair<double, std::set<int>> baseCase(double B, const std::set<int>& S) {
        if (S.empty()) {
            return {B, {}};
        }

        int x = *S.begin();
        std::set<int> U0;
        U0.insert(x);

        // Priority queue: (distance, vertex)
        using PQEntry = std::pair<double, int>;
        std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<PQEntry>> H;
        std::set<int> in_heap;

        H.push({d_hat[x], x});
        in_heap.insert(x);

        while (!H.empty() && static_cast<int>(U0.size()) < k + 1) {
            auto [dist, u] = H.top();
            H.pop();

            if (dist > d_hat[u]) continue;  // Outdated entry

            U0.insert(u);
            complete[u] = true;

            for (const auto& [v, w] : graph.adj[u]) {
                relaxation_count++;
                if (d_hat[u] + w <= d_hat[v] && d_hat[u] + w < B) {
                    d_hat[v] = d_hat[u] + w;
                    pred[v] = u;

                    H.push({d_hat[v], v});
                    in_heap.insert(v);
                }
            }
        }

        if (static_cast<int>(U0.size()) <= k) {
            return {B, U0};
        } else {
            // Find max distance and return B' = max distance
            double max_dist = 0;
            for (int v : U0) {
                max_dist = std::max(max_dist, d_hat[v]);
            }

            std::set<int> U_result;
            for (int v : U0) {
                if (d_hat[v] < max_dist) {
                    U_result.insert(v);
                }
            }
            return {max_dist, U_result};
        }
    }

    // Find Pivots (Algorithm 1)
    std::pair<std::set<int>, std::set<int>> findPivots(double B, const std::set<int>& S) {
        std::set<int> W = S;
        std::set<int> Wi_prev = S;

        // Relax for k steps
        for (int i = 0; i < k; ++i) {
            std::set<int> Wi;

            for (int u : Wi_prev) {
                for (const auto& [v, w] : graph.adj[u]) {
                    relaxation_count++;
                    if (d_hat[u] + w <= d_hat[v]) {
                        d_hat[v] = d_hat[u] + w;
                        pred[v] = u;

                        if (d_hat[u] + w < B) {
                            Wi.insert(v);
                        }
                    }
                }
            }

            for (int v : Wi) {
                W.insert(v);
            }

            // Early termination if W is too large
            if (static_cast<int>(W.size()) > k * static_cast<int>(S.size())) {
                // Return P = S
                return {S, W};
            }

            Wi_prev = Wi;
        }

        // Build forest F and find pivots
        // F = edges (u,v) where u,v in W and d_hat[v] = d_hat[u] + w
        std::map<int, std::vector<int>> children;  // parent -> children
        std::map<int, int> parent;
        std::set<int> roots;

        for (int v : W) {
            if (S.count(v)) {
                roots.insert(v);
            }
        }

        // Build tree structure based on predecessors
        for (int v : W) {
            if (pred[v] >= 0 && W.count(pred[v])) {
                children[pred[v]].push_back(v);
                parent[v] = pred[v];
            } else if (S.count(v)) {
                // v is a root
            }
        }

        // Count subtree sizes for vertices in S
        std::map<int, int> subtree_size;

        // DFS to compute subtree sizes
        std::function<int(int)> computeSize = [&](int v) -> int {
            int size = 1;
            for (int child : children[v]) {
                if (W.count(child)) {
                    size += computeSize(child);
                }
            }
            subtree_size[v] = size;
            return size;
        };

        for (int root : roots) {
            computeSize(root);
        }

        // Find pivots: vertices in S with subtree size >= k
        std::set<int> P;
        for (int u : S) {
            if (subtree_size[u] >= k) {
                P.insert(u);
            }
        }

        // If P is empty but S is not, include at least one pivot
        if (P.empty() && !S.empty()) {
            P.insert(*S.begin());
        }

        // Mark vertices in W as complete if they're fully resolved
        for (int v : W) {
            // A vertex is complete if we've fully explored its shortest path
            // For simplicity, mark those that were visited in k steps
            complete[v] = true;
        }

        return {P, W};
    }
};

// Convenience function
inline NewSSSP::Result computeNewSSSP(const SimpleGraph& graph, int source) {
    NewSSSP solver(graph);
    return solver.solve(source);
}

}  // namespace sssp

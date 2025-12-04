#pragma once

#include "graph_types.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <iostream>
#include <algorithm>

namespace sssp {

/**
 * Parser for Matrix Market (.mtx) files
 * Supports:
 * - Coordinate format (sparse)
 * - Real and integer weights
 * - Symmetric and general matrices
 * - Pattern matrices (no weights - uses weight 1.0)
 */
class MTXParser {
public:
    struct GraphInfo {
        int num_nodes;
        int num_edges;
        bool is_symmetric;
        bool is_pattern;  // No weights in file
        bool is_directed;
    };

    static std::pair<SimpleGraph, GraphInfo> parse(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filepath);
        }

        GraphInfo info = {};
        info.is_symmetric = false;
        info.is_pattern = false;
        info.is_directed = true;

        std::string line;

        // Read header line
        if (!std::getline(file, line)) {
            throw std::runtime_error("Empty MTX file");
        }

        // Parse header: %%MatrixMarket matrix coordinate [real|integer|pattern] [general|symmetric]
        if (line.substr(0, 14) != "%%MatrixMarket") {
            throw std::runtime_error("Invalid MTX header: " + line);
        }

        std::string lower_line = line;
        std::transform(lower_line.begin(), lower_line.end(), lower_line.begin(), ::tolower);

        if (lower_line.find("symmetric") != std::string::npos) {
            info.is_symmetric = true;
            info.is_directed = false;
        }

        if (lower_line.find("pattern") != std::string::npos) {
            info.is_pattern = true;
        }

        // Skip comment lines
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '%') {
                continue;
            }
            break;
        }

        // Parse dimensions: rows cols entries
        int rows, cols, entries;
        std::istringstream dim_stream(line);
        if (!(dim_stream >> rows >> cols >> entries)) {
            throw std::runtime_error("Invalid dimension line: " + line);
        }

        // For graph, we use max(rows, cols) as number of nodes
        info.num_nodes = std::max(rows, cols);
        info.num_edges = entries;

        SimpleGraph graph(info.num_nodes);

        // Read edges
        int edges_read = 0;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '%') {
                continue;
            }

            std::istringstream edge_stream(line);
            int u, v;
            double w = 1.0;

            if (!(edge_stream >> u >> v)) {
                continue;  // Skip malformed lines
            }

            // MTX uses 1-based indexing
            u--;
            v--;

            if (u < 0 || u >= info.num_nodes || v < 0 || v >= info.num_nodes) {
                continue;  // Skip out-of-range edges
            }

            // Read weight if not pattern
            if (!info.is_pattern) {
                if (!(edge_stream >> w)) {
                    w = 1.0;  // Default weight
                }
                // Handle negative weights by taking absolute value
                if (w < 0) {
                    w = -w;
                }
                // Handle zero weights
                if (w == 0) {
                    w = 1.0;
                }
            }

            graph.add_edge(u, v, w);
            edges_read++;

            // For symmetric matrices, add reverse edge (if not self-loop)
            if (info.is_symmetric && u != v) {
                graph.add_edge(v, u, w);
            }
        }

        // Update actual edge count
        info.num_edges = graph.m;

        return {graph, info};
    }

    // Parse and print info
    static void printInfo(const std::string& filepath) {
        auto [graph, info] = parse(filepath);

        std::cout << "MTX File: " << filepath << "\n";
        std::cout << "  Nodes: " << info.num_nodes << "\n";
        std::cout << "  Edges: " << info.num_edges << "\n";
        std::cout << "  Type: " << (info.is_directed ? "Directed" : "Undirected") << "\n";
        std::cout << "  Weights: " << (info.is_pattern ? "None (using 1.0)" : "Yes") << "\n";
    }
};

}  // namespace sssp

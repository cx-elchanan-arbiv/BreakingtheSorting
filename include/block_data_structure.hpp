#pragma once

#include <vector>
#include <list>
#include <set>
#include <map>
#include <algorithm>
#include <cmath>
#include <limits>

namespace sssp {

/**
 * Block-based data structure from Lemma 3.3 of the paper.
 * Supports:
 * - Insert: O(max{1, log(N/M)}) amortized
 * - BatchPrepend: O(L * max{1, log(L/M)}) amortized
 * - Pull: O(|S'|) amortized - returns M smallest elements
 */
class BlockDataStructure {
public:
    using KeyValue = std::pair<int, double>;  // (vertex_id, distance)

private:
    int M;           // Block size parameter
    double B;        // Upper bound on values
    int N;           // Maximum expected insertions

    // Block structure: list of (key, value) pairs, sorted by value within block
    struct Block {
        std::list<KeyValue> elements;
        double upper_bound;  // Upper bound for elements in this block

        Block() : upper_bound(std::numeric_limits<double>::infinity()) {}
        explicit Block(double ub) : upper_bound(ub) {}
    };

    // D0: blocks from batch prepends (at front)
    std::list<Block> D0;
    // D1: blocks from regular inserts
    std::list<Block> D1;

    // Track which keys exist and their current value
    std::map<int, double> key_values;

    // Upper bounds for D1 blocks (for binary search)
    std::set<double> d1_upper_bounds;

public:
    BlockDataStructure() : M(1), B(std::numeric_limits<double>::infinity()), N(0) {}

    void initialize(int m, double b, int max_n = 0) {
        M = std::max(1, m);
        B = b;
        N = max_n > 0 ? max_n : m * 10;
        D0.clear();
        D1.clear();
        key_values.clear();
        d1_upper_bounds.clear();

        // Initialize D1 with one empty block with upper bound B
        D1.emplace_back(B);
        d1_upper_bounds.insert(B);
    }

    bool empty() const {
        for (const auto& block : D0) {
            if (!block.elements.empty()) return false;
        }
        for (const auto& block : D1) {
            if (!block.elements.empty()) return false;
        }
        return true;
    }

    size_t size() const {
        return key_values.size();
    }

    void insert(int key, double value) {
        // Check if key exists
        auto it = key_values.find(key);
        if (it != key_values.end()) {
            if (value < it->second) {
                // Remove old entry and insert new one
                removeKey(key);
            } else {
                return;  // Keep existing smaller value
            }
        }

        key_values[key] = value;

        // Find appropriate block in D1
        auto block_it = findBlockForValue(value);
        block_it->elements.emplace_back(key, value);

        // Split if needed
        if (static_cast<int>(block_it->elements.size()) > M) {
            splitBlock(block_it);
        }
    }

    void batchPrepend(std::vector<KeyValue>& items) {
        if (items.empty()) return;

        // Remove duplicates, keeping smallest value per key
        std::map<int, double> unique_items;
        for (const auto& [key, value] : items) {
            auto it = unique_items.find(key);
            if (it == unique_items.end() || value < it->second) {
                unique_items[key] = value;
            }
        }

        // Check against existing keys
        std::vector<KeyValue> to_add;
        for (const auto& [key, value] : unique_items) {
            auto it = key_values.find(key);
            if (it == key_values.end()) {
                to_add.emplace_back(key, value);
                key_values[key] = value;
            } else if (value < it->second) {
                removeKey(key);
                to_add.emplace_back(key, value);
                key_values[key] = value;
            }
        }

        if (to_add.empty()) return;

        // Sort by value
        std::sort(to_add.begin(), to_add.end(),
                  [](const auto& a, const auto& b) { return a.second < b.second; });

        int L = static_cast<int>(to_add.size());
        if (L <= M) {
            // Create single block
            Block new_block;
            for (const auto& kv : to_add) {
                new_block.elements.push_back(kv);
            }
            if (!to_add.empty()) {
                new_block.upper_bound = to_add.back().second;
            }
            D0.push_front(new_block);
        } else {
            // Create multiple blocks
            int num_blocks = (L + M / 2 - 1) / (M / 2);
            int per_block = (L + num_blocks - 1) / num_blocks;

            std::list<Block> new_blocks;
            for (int i = 0; i < L; i += per_block) {
                Block block;
                int end = std::min(i + per_block, L);
                for (int j = i; j < end; ++j) {
                    block.elements.push_back(to_add[j]);
                }
                if (!block.elements.empty()) {
                    block.upper_bound = block.elements.back().second;
                }
                new_blocks.push_back(block);
            }

            // Prepend all new blocks
            D0.splice(D0.begin(), new_blocks);
        }
    }

    // Pull returns up to M smallest elements and the separating bound
    std::pair<std::vector<int>, double> pull() {
        std::vector<KeyValue> candidates;
        double sep_bound = B;

        // Collect from D0
        int collected_d0 = 0;
        for (auto& block : D0) {
            for (const auto& elem : block.elements) {
                candidates.push_back(elem);
                collected_d0++;
            }
            if (collected_d0 >= M) break;
        }

        // Collect from D1
        int collected_d1 = 0;
        for (auto& block : D1) {
            for (const auto& elem : block.elements) {
                candidates.push_back(elem);
                collected_d1++;
            }
            if (collected_d1 >= M) break;
        }

        if (candidates.empty()) {
            return {{}, B};
        }

        // Sort all candidates
        std::sort(candidates.begin(), candidates.end(),
                  [](const auto& a, const auto& b) { return a.second < b.second; });

        // Take at most M
        int take = std::min(M, static_cast<int>(candidates.size()));
        std::vector<int> result;
        result.reserve(take);

        for (int i = 0; i < take; ++i) {
            result.push_back(candidates[i].first);
        }

        // Remove taken elements
        for (int i = 0; i < take; ++i) {
            removeKey(candidates[i].first);
        }

        // Determine separator bound
        if (static_cast<int>(candidates.size()) > take) {
            sep_bound = candidates[take].second;
        } else if (empty()) {
            sep_bound = B;
        } else {
            // Find minimum remaining value
            sep_bound = B;
            for (const auto& block : D0) {
                for (const auto& elem : block.elements) {
                    sep_bound = std::min(sep_bound, elem.second);
                }
            }
            for (const auto& block : D1) {
                for (const auto& elem : block.elements) {
                    sep_bound = std::min(sep_bound, elem.second);
                }
            }
        }

        return {result, sep_bound};
    }

    // Get value for a key (for debugging/testing)
    double getValue(int key) const {
        auto it = key_values.find(key);
        if (it != key_values.end()) {
            return it->second;
        }
        return std::numeric_limits<double>::infinity();
    }

private:
    void removeKey(int key) {
        auto kv_it = key_values.find(key);
        if (kv_it == key_values.end()) return;

        double value = kv_it->second;
        key_values.erase(kv_it);

        // Remove from D0
        for (auto& block : D0) {
            for (auto it = block.elements.begin(); it != block.elements.end(); ++it) {
                if (it->first == key) {
                    block.elements.erase(it);
                    return;
                }
            }
        }

        // Remove from D1
        for (auto& block : D1) {
            for (auto it = block.elements.begin(); it != block.elements.end(); ++it) {
                if (it->first == key) {
                    block.elements.erase(it);
                    return;
                }
            }
        }
    }

    std::list<Block>::iterator findBlockForValue(double value) {
        // Find block in D1 with smallest upper_bound >= value
        for (auto it = D1.begin(); it != D1.end(); ++it) {
            if (it->upper_bound >= value) {
                return it;
            }
        }
        // Return last block
        return std::prev(D1.end());
    }

    void splitBlock(std::list<Block>::iterator block_it) {
        auto& block = *block_it;
        if (static_cast<int>(block.elements.size()) <= M) return;

        // Convert to vector for median finding
        std::vector<KeyValue> elems(block.elements.begin(), block.elements.end());
        std::sort(elems.begin(), elems.end(),
                  [](const auto& a, const auto& b) { return a.second < b.second; });

        int mid = elems.size() / 2;

        // Create two new blocks
        Block block1, block2;
        for (int i = 0; i < mid; ++i) {
            block1.elements.push_back(elems[i]);
        }
        block1.upper_bound = elems[mid - 1].second;

        for (size_t i = mid; i < elems.size(); ++i) {
            block2.elements.push_back(elems[i]);
        }
        block2.upper_bound = block.upper_bound;

        // Update D1
        d1_upper_bounds.erase(block.upper_bound);
        d1_upper_bounds.insert(block1.upper_bound);
        d1_upper_bounds.insert(block2.upper_bound);

        auto next_it = std::next(block_it);
        D1.erase(block_it);
        D1.insert(next_it, block1);
        D1.insert(next_it, block2);
    }
};

}  // namespace sssp

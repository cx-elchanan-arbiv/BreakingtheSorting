# Breaking the Sorting Barrier for Directed Single-Source Shortest Paths

Implementation of the O(m log^{2/3} n) SSSP algorithm from the paper:

> **Breaking the Sorting Barrier for Directed Single-Source Shortest Paths**
> Ran Duan, Jiayi Mao, Xiao Mao, Xinkai Shu, Longhui Yin (2025)
> arXiv:2504.17033v2

## Overview

This project implements the first deterministic algorithm that breaks the O(m + n log n) time bound of Dijkstra's algorithm for single-source shortest paths on sparse directed graphs with real non-negative edge weights.

### Key Result

- **New Algorithm**: O(m log^{2/3} n) time
- **Dijkstra's Algorithm**: O(m + n log n) time (with Fibonacci heap)

For sparse graphs where m = O(n), the new algorithm achieves O(n log^{2/3} n) vs Dijkstra's O(n log n).

## Quick Start

```bash
# Install CMake (if not installed)
brew install cmake    # macOS

# Build
cd BreakingtheSorting
./build.sh

# Run benchmark on your MTX file
./build/sssp_benchmark /path/to/your/graph.mtx
```

## Building

### Prerequisites

- CMake 3.16+
- C++17 compatible compiler
- LEMON graph library (optional, will be downloaded if not found)

### Build Commands

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

Or simply:
```bash
./build.sh
```

### Running Tests

```bash
cd build
ctest --output-on-failure
# or
./sssp_tests
```

## Main Tool: MTX Benchmark

The main executable reads a graph from an MTX file and compares the performance of LEMON's Dijkstra vs the new algorithm.

### Usage

```bash
./sssp_benchmark <mtx_file> [num_runs] [source_node]
```

**Arguments:**
- `mtx_file` - Path to Matrix Market (.mtx) graph file (required)
- `num_runs` - Number of benchmark runs (default: 5)
- `source_node` - Source node for SSSP (default: 0)

### Example

```bash
./sssp_benchmark ~/graphs/roadNet-CA.mtx 10 0
```

### Example Output

```
======================================================================
  SSSP Benchmark: New Algorithm vs LEMON Dijkstra
======================================================================

Loading graph from: /path/to/graph.mtx

----------------------------------------------------------------------
Graph Statistics:
----------------------------------------------------------------------
  Nodes:              1965206
  Edges:              5533214
  Density:           0.000001
  Avg degree:            2.82
  Type:               Directed
  Source node:              0
  Benchmark runs:          10

----------------------------------------------------------------------
Theoretical Complexity:
----------------------------------------------------------------------
  Dijkstra:     O(m + n log n) = O(2.65e+07)
  New SSSP:     O(m log^{2/3} n) = O(1.89e+07)
  Ratio:        1.403x (theoretical)

----------------------------------------------------------------------
Running warmup...
----------------------------------------------------------------------
  Dijkstra reachable: 1957027 / 1965206
  New SSSP reachable: 1957027 / 1965206
  Max error:          0.00e+00
  Correctness:        PASSED

----------------------------------------------------------------------
Running benchmark (10 iterations)...
----------------------------------------------------------------------
  Run 1/10... Dijkstra: 245.32ms, LEMON: 198.45ms, New: 312.18ms
  Run 2/10... Dijkstra: 243.18ms, LEMON: 196.22ms, New: 308.45ms
  ...

======================================================================
  BENCHMARK RESULTS
======================================================================

Algorithm            Mean (ms)     Median    Std Dev        Min        Max
----------------------------------------------------------------------
Dijkstra (simple)       248.12     245.32       5.23     242.11     256.78
LEMON Dijkstra          201.34     198.45       4.12     196.22     208.91
New SSSP                315.67     312.18       8.45     305.12     328.34

----------------------------------------------------------------------
Speedup Analysis (based on median times):
----------------------------------------------------------------------
  Dijkstra / New SSSP: 0.786x (Dijkstra is faster)
  LEMON / New SSSP:    0.636x (LEMON is faster)

======================================================================
  SUMMARY
======================================================================

  Graph:          /path/to/graph.mtx
  Size:           1965206 nodes, 5533214 edges
  Best time:      LEMON Dijkstra (198.45 ms)
```

## Other Executables

### Google Benchmarks (Synthetic Graphs)

```bash
./build/sssp_benchmarks
```

Runs comprehensive benchmarks on automatically generated graphs of various types and sizes.

### Demo

```bash
./build/sssp_demo [num_nodes] [num_edges]
```

Runs a quick comparison on randomly generated graphs.

## Project Structure

```
.
├── CMakeLists.txt              # Build configuration
├── build.sh                    # Build script
├── setup.sh                    # Dependency setup script
├── README.md                   # This file
├── include/
│   ├── graph_types.hpp         # Graph data structures
│   ├── graph_generator.hpp     # Random graph generators
│   ├── mtx_parser.hpp          # Matrix Market file parser
│   ├── block_data_structure.hpp # Lemma 3.3 data structure
│   ├── new_sssp.hpp            # Main algorithm implementation
│   └── dijkstra_lemon.hpp      # LEMON Dijkstra wrapper
├── src/
│   ├── main.cpp                # Demo application
│   └── sssp_benchmark.cpp      # Main CLI benchmark tool
├── tests/
│   ├── test_main.cpp           # Test entry point
│   ├── test_graph.cpp          # Graph tests
│   ├── test_dijkstra.cpp       # Dijkstra tests
│   ├── test_new_sssp.cpp       # New algorithm tests
│   └── test_correctness.cpp    # Correctness comparison tests
└── benchmarks/
    └── benchmark_main.cpp      # Google Benchmark benchmarks
```

## MTX File Format

The benchmark tool supports Matrix Market (.mtx) format, commonly used for sparse graphs.

### Where to Download Test Graphs

- [SuiteSparse Matrix Collection](https://sparse.tamu.edu/) - Large collection of sparse matrices
- [SNAP Datasets](https://snap.stanford.edu/data/) - Social and information networks
- [Network Repository](https://networkrepository.com/) - Network datasets

### Example MTX File

```
%%MatrixMarket matrix coordinate real general
4 4 5
1 2 1.0
1 3 2.0
2 3 1.5
2 4 3.0
3 4 1.0
```

### Supported Formats

- **coordinate** format (sparse)
- **real**, **integer**, and **pattern** (no weights) types
- **general** and **symmetric** matrices
- Automatically handles 1-based to 0-based index conversion

## Algorithm Overview

The algorithm combines two classical approaches:

1. **Dijkstra's Algorithm**: Uses a priority queue, sorts vertices by distance
2. **Bellman-Ford Algorithm**: Dynamic programming, no sorting needed

### Key Ideas

1. **Frontier Reduction**: Instead of maintaining all frontier vertices, reduce to "pivots" - vertices with large shortest-path subtrees
2. **FindPivots (Algorithm 1)**: Runs k Bellman-Ford relaxation steps, identifies pivots with subtree size ≥ k
3. **BMSSP (Algorithm 3)**: Bounded Multi-Source Shortest Path - recursive divide-and-conquer
4. **Block Data Structure (Lemma 3.3)**: Efficient partial sorting with batch prepend support

### Parameters

- k = ⌊log^{1/3}(n)⌋
- t = ⌊log^{2/3}(n)⌋
- Recursion depth: O(log n / t) = O(log^{1/3} n)

## Implementation Details

### Files Description

| File | Description |
|------|-------------|
| `mtx_parser.hpp` | Parser for Matrix Market files, supports symmetric/general, weighted/unweighted |
| `new_sssp.hpp` | The new O(m log^{2/3} n) algorithm implementation |
| `dijkstra_lemon.hpp` | Wrapper around LEMON's optimized Dijkstra + simple Dijkstra implementation |
| `block_data_structure.hpp` | Block-based data structure from Lemma 3.3 with Insert, BatchPrepend, Pull |
| `graph_generator.hpp` | Generators for random sparse, grid, complete, and scale-free graphs |
| `sssp_benchmark.cpp` | Main CLI tool that reads MTX and runs benchmarks |

### Benchmark Comparisons

The benchmarks compare three implementations:

1. **Simple Dijkstra** - Standard implementation with binary heap (std::priority_queue)
2. **LEMON Dijkstra** - Highly optimized implementation from LEMON library
3. **New SSSP** - Our implementation of the paper's algorithm

Graph types tested:
- Sparse random graphs (m = O(n))
- Very sparse graphs (m ≈ n)
- Dense graphs (m = O(n^1.5))
- Grid graphs
- Scale-free graphs (Barabási-Albert model)

## Notes

This is a research implementation focusing on correctness over absolute performance. The theoretical advantage of O(m log^{2/3} n) vs O(m + n log n) may not manifest in practice until n is very large due to:

1. **Constant factors** in the new algorithm are larger
2. **Cache efficiency** of simple Dijkstra is better
3. **Modern heap implementations** are highly optimized

The primary value is in demonstrating that Dijkstra's algorithm is **not optimal** for SSSP.

### When Does the New Algorithm Win?

Theoretically, the new algorithm is faster when:
- The graph is **very sparse** (m close to n)
- n is **extremely large** (millions of nodes)
- The comparison-addition model accurately reflects the computational cost

In practice, the crossover point may be at graph sizes larger than fit in memory due to the constant factors involved.

## References

- [arXiv:2504.17033v2](https://arxiv.org/abs/2504.17033) - Original paper
- [LEMON Graph Library](https://lemon.cs.elte.hu/) - Library for Efficient Modeling and Optimization in Networks
- [Google Test](https://github.com/google/googletest) - Testing framework
- [Google Benchmark](https://github.com/google/benchmark) - Benchmarking framework
- [Matrix Market](https://math.nist.gov/MatrixMarket/) - File format specification

## License

MIT License

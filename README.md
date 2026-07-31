# CacheCraft Simulator

CacheCraft is a C++17 cache-architecture simulator for exploring how capacity, block size, associativity, replacement policy, and write strategy affect memory-system behaviour. Version 2 expands the original LRU-only demonstration into a small research and teaching tool with typed traces, miss classification, policy comparison, dirty-line handling, hierarchy support, JSON output, and inspectable cache state.

## Simulation features

### Configurable organization

- arbitrary capacity, power-of-two block size, and associativity
- direct-mapped through fully associative organizations
- validated geometry and latency inputs
- deterministic random-policy experiments through an explicit seed

### Replacement policies

- least recently used (`lru`)
- first in, first out (`fifo`)
- deterministic pseudo-random (`random`)

### Write behaviour

- write-back or write-through
- write-allocate or no-write-allocate
- dirty-bit tracking
- writeback, memory-read, memory-write, and bytes-fetched accounting

### 3C miss model

A fully associative shadow cache with the same line count classifies every miss as:

- **compulsory**: the block has never been referenced
- **conflict**: the block would still fit in a fully associative cache but was displaced by set mapping
- **capacity**: the working set exceeded total cache capacity

### Trace support

Accepted forms include:

```text
0x1000
R 0x1000
W,0x2000,8
I 4096 4
```

`R`, `W`, and `I` represent data reads, data writes, and instruction fetches. An optional size expands accesses that cross cache-block boundaries. Blank lines, comments, inline comments, decimal addresses, and hexadecimal addresses are accepted.

### Analysis output

- access counts by type
- hits, misses, evictions, and hit/miss rates
- compulsory, conflict, and capacity misses
- memory reads, memory writes, writebacks, and bytes fetched
- average memory access time from configurable hit latency and miss penalty
- JSON output for automation
- CSV-style comparison across LRU, FIFO, and random policies
- per-set line dumps including valid, dirty, tag, last-used, and insertion timestamps

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Run

The original positional interface still works:

```bash
./build/cachecraft_cli 32768 64 8 examples/sample.trace
```

The expanded interface is better for experiments:

```bash
./build/cachecraft_cli \
  --trace examples/matrix.trace \
  --capacity 32768 \
  --block 64 \
  --ways 8 \
  --policy lru \
  --write-policy write-back \
  --allocation write-allocate \
  --hit-latency 1 \
  --miss-penalty 80
```

Inspect every access and the final state of set zero:

```bash
./build/cachecraft_cli --trace examples/sample.trace --capacity 256 --block 16 --ways 2 --verbose --dump-set 0
```

Emit machine-readable JSON:

```bash
./build/cachecraft_cli --trace examples/sample.trace --json
```

Compare all replacement policies with identical geometry and trace:

```bash
./build/cachecraft_cli --trace examples/matrix.trace --capacity 256 --block 16 --ways 2 --compare
```

## Library example

```cpp
#include "cache.hpp"

cachecraft::CacheConfig config;
config.capacity_bytes = 32 * 1024;
config.block_bytes = 64;
config.ways = 8;
config.replacement = cachecraft::ReplacementPolicy::LRU;
config.write_policy = cachecraft::WritePolicy::WriteBack;

cachecraft::Cache cache(config);
auto result = cache.access(0x1000, cachecraft::AccessType::Write);
const auto& stats = cache.statistics();
```

`TwoLevelHierarchy` is also available for L1/L2 experiments. L2 is accessed only when L1 misses, and both levels retain independent statistics.

## Project structure

```text
include/
├── cache.hpp   # cache model, policies, results, hierarchy
└── trace.hpp   # typed trace records and parser API
src/
├── cache.cpp   # policy engine, 3C classifier, hierarchy
├── trace.cpp   # robust trace parsing
└── main.cpp    # experiment-oriented command-line interface
tests/
└── test_cache.cpp
examples/
├── sample.trace
└── matrix.trace
```

## Engineering focus

CacheCraft is intentionally dependency-free and uses deterministic tests. The implementation emphasizes explicit domain types, invariant validation, policy separation, testable state transitions, useful measurement, and compatibility with both interactive learning and scripted experiments.

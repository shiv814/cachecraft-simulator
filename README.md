# CacheCraft Simulator

CacheCraft is a C++17 command-line simulator for direct-mapped and set-associative CPU caches. It models configurable capacity, block size, associativity, and least-recently-used replacement.

## Build and test

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Run

```bash
./build/cachecraft_cli 32768 64 8 examples/sample.trace
```

Arguments are cache capacity in bytes, block size in bytes, associativity, and a hexadecimal address trace file.

## Design

An address is split into block number, set index, and tag. Each set tracks valid lines and a monotonic access timestamp so the simulator can select the least-recently-used victim when a set is full.

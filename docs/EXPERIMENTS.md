# Running Cache Experiments

Build once, then run the dedicated experiment executable:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/cachecraft_experiments > experiments.csv
```

The built-in workload uses a deterministic hot-set generator, then sweeps capacity and associativity. Because the seed is fixed, results are reproducible.

## Useful experimental questions

- How quickly does miss rate flatten as capacity increases?
- Does associativity reduce conflict misses enough to justify additional hardware complexity?
- Which configurations lie on the capacity/miss/traffic Pareto frontier?
- How does a sequential workload differ from a hot-set workload in reuse distance?
- How much traffic is caused by write-through versus write-back policy?

## Methodology

Keep one variable controlled when possible. Use the same input trace across compared configurations, preserve the deterministic seed for synthetic workloads, and report both performance (`AMAT`, miss rate) and cost proxies (capacity, traffic) rather than only one number.

# Contributing

- keep cache-state mutation inside the cache core
- keep workload/experiment analysis independent from CLI formatting
- prefer deterministic tests and fixed seeds
- build Debug and Release configurations
- run `ctest --output-on-failure` after every behavior change
- document new metrics and their interpretation

The project targets standard C++17 and treats compiler warnings as issues to fix rather than ignore.

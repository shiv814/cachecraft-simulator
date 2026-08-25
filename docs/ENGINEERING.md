# Engineering Notes

## Why deterministic synthetic traces?

They are not a replacement for application traces. They provide small, reproducible workloads that make cache behavior easy to isolate in tests and demonstrations.

## Why reuse distance?

Hit/miss totals explain **what** happened for one cache. Reuse distance helps explain **why** a workload is cache-friendly or cache-hostile independently of a single configuration.

## Why Pareto frontiers?

Architectural choices trade silicon/storage cost against latency and memory traffic. A Pareto frontier preserves those trade-offs rather than hiding them behind arbitrary weights.

## Correctness boundaries

CacheCraft models functional cache-policy behavior and timing using configurable hit/miss latency parameters. It does not claim cycle-accurate CPU simulation, coherence, prefetching, out-of-order execution or a physical area/power model.

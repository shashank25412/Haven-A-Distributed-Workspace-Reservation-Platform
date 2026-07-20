---
title: Haven Engineering Design Documentation
document: 13 - Performance
version: 1.0
status: Draft
author: Shashank Kumar
reviewer: Kumar Rahul
last_updated: 2026-07-20
related_documents:
  - 01-requirements.md
  - 02-high-level-design.md
  - 06-database-design.md
  - 08-concurrency.md
  - 09-caching.md
  - 11-observability.md
  - 12-testing.md
related_adrs:
  - ADR-001-ModularMonolith
  - ADR-002-Couchbase
  - ADR-003-Drogon
  - ADR-004-OptimisticLocking
  - ADR-006-Redis
---

# Haven — Performance Design

## 1. Overview

This document defines Haven's performance objectives, workload assumptions, bottleneck hypotheses, benchmarking strategy, capacity-planning model, and optimization process.

Performance is treated as an engineering property that must be measured with representative workloads. Haven does not claim production-scale throughput based only on framework benchmarks or synthetic microbenchmarks.

The central performance requirement is:

> Improve latency and throughput without weakening reservation correctness, tenant isolation, or failure safety.

---

## 2. Goals

- Establish measurable latency and throughput objectives.
- Identify performance-sensitive request paths.
- Define realistic benchmark datasets and traffic profiles.
- Prevent accidental algorithmic or query regressions.
- Measure contention separately from normal throughput.
- Keep retries, cache behavior, and dependency latency visible.
- Support repeatable local and CI benchmarks.
- Define when optimization is justified.
- Provide a path for capacity planning and horizontal scaling.

---

## 3. Non-Goals

This document does not promise:

- A formal external service-level agreement.
- Multi-region performance.
- Internet-scale traffic in the MVP.
- Zero-latency cache consistency.
- Exactly-once event processing.
- Performance parity with a specialized in-memory scheduler.
- Optimization before functional correctness is complete.

---

## 4. Performance Principles

1. Correctness precedes speed.
2. Measure before optimizing.
3. Optimize end-to-end bottlenecks, not isolated code aesthetics.
4. Use representative data volume and skew.
5. Track percentile latency rather than averages alone.
6. Separate business conflicts from technical failures.
7. Measure retry amplification.
8. Treat hot-resource contention as a separate workload.
9. Prefer bounded work and pagination.
10. Preserve readability unless evidence justifies complexity.
11. Re-run benchmarks after every meaningful optimization.
12. Record benchmark environment and configuration.

---

## 5. Initial Performance Objectives

These values are engineering targets for a healthy initial deployment. They are not external contractual SLAs.

| Operation | p50 | p95 | p99 |
|---|---:|---:|---:|
| Liveness | < 3 ms | < 10 ms | < 20 ms |
| Readiness | < 20 ms | < 75 ms | < 150 ms |
| Resource detail | < 20 ms | < 50 ms | < 100 ms |
| Search resources | < 60 ms | < 150 ms | < 300 ms |
| Create reservation | < 80 ms | < 200 ms | < 400 ms |
| Get reservation | < 30 ms | < 75 ms | < 150 ms |
| Cancel reservation | < 60 ms | < 150 ms | < 300 ms |
| Extend reservation | < 80 ms | < 200 ms | < 400 ms |
| Approve reservation | < 80 ms | < 200 ms | < 400 ms |

Targets assume:

- Healthy Couchbase.
- Healthy network.
- Correct indexes.
- Bounded payloads.
- No extreme resource contention.
- Warm application process.
- Representative but not overloaded traffic.

---

## 6. Initial Workload Assumptions

| Dimension | Initial Assumption |
|---|---:|
| Organizations | 2–3 |
| Total resources | Up to 50,000 |
| Reservations per day | 10,000–100,000 |
| Daily active users | 1,000–10,000 |
| Peak concurrent users | 500–2,000 |
| Peak search requests/sec | 500–2,000 |
| Peak create requests/sec | 50–200 |
| Search-to-create ratio | Approximately 10:1 |
| Typical page size | 20 |
| Maximum page size | 100 |
| Standard duration | Up to 12 hours |
| Maintenance duration | Up to 24 hours |

The data distribution is assumed to be skewed:

- Most resources have low contention.
- A small subset of resources are hot.
- Resource metadata changes infrequently.
- Reservation history grows continuously.
- Search traffic substantially exceeds write traffic.

---

## 7. Future Benchmark Profiles

Future performance testing should model:

| Profile | Resources | Reservations | Organizations | Characteristic |
|---|---:|---:|---:|---|
| Small | 5,000 | 100,000 | 3 | Local development |
| Medium | 50,000 | 5,000,000 | 100 | Initial production-like |
| Large | 1,000,000 | 100,000,000 | 10,000 | Evolution target |
| Hot-resource | 50,000 | 5,000,000 | 100 | High contention on 1% of resources |
| Search-heavy | 1,000,000 | 100,000,000 | 10,000 | Broad filtered discovery |
| Event-heavy | 50,000 | 10,000,000 | 1,000 | Consumer and outbox stress |

Large profiles do not imply the MVP is certified for that scale. They expose architectural limits.

---

## 8. Critical Request Paths

### 8.1 Resource Search

```text
HTTP parsing
→ JWT validation
→ query validation
→ optional Redis lookup
→ Couchbase resource candidate query
→ blocking reservation query
→ in-process exclusion
→ sorting and pagination
→ response serialization
```

Primary risks:

- Large candidate sets.
- Ineffective Couchbase indexes.
- Large `IN` predicates.
- Offset pagination at high page numbers.
- Repeated feature filtering.
- Redis timeout increasing tail latency.
- Returning too much metadata.

### 8.2 Create Reservation

```text
HTTP parsing
→ JWT validation
→ idempotency lookup
→ resource lookup
→ organization policy lookup
→ interval and policy validation
→ Couchbase transaction
→ schedule guard read/update
→ reservation insert
→ outbox insert
→ idempotency completion
→ response serialization
```

Primary risks:

- Transaction retries.
- Hot guard documents.
- Cross-midnight multi-document transactions.
- Couchbase network latency.
- Large transaction timeout.
- Unnecessary reads inside transaction.
- Idempotency contention.
- Synchronous event publication accidentally entering the path.

### 8.3 Approval

```text
authorization
→ reservation lookup
→ conflict recheck
→ guard update
→ reservation update
→ outbox insert
```

Primary risks:

- Approval delay causing a higher conflict probability.
- Concurrent cancellation.
- Repeated transaction retries.
- Expensive authorization lookup.

### 8.4 Event Publication

```text
outbox query
→ claim with CAS
→ serialize
→ Kafka publish
→ mark published
```

Primary risks:

- Large outbox backlog.
- Broker latency.
- Claim contention.
- Poison events.
- Unbounded retry.

---

## 9. Performance Budget

A performance budget allocates approximate latency to components.

### 9.1 Create Reservation p95 Budget

| Component | Budget |
|---|---:|
| HTTP parsing and validation | 10 ms |
| Authentication and authorization | 15 ms |
| Resource and policy retrieval | 30 ms |
| Transaction and guard operations | 100 ms |
| Serialization and response | 10 ms |
| Retry and network margin | 35 ms |
| **Total** | **200 ms** |

### 9.2 Search p95 Budget

| Component | Budget |
|---|---:|
| HTTP/auth/validation | 15 ms |
| Cache lookup | 5 ms |
| Resource candidate query | 55 ms |
| Blocking reservation query | 45 ms |
| In-process filtering/pagination | 15 ms |
| Serialization and margin | 15 ms |
| **Total** | **150 ms** |

These budgets guide diagnosis. They are revised after measurement.

---

## 10. Algorithmic Complexity

### 10.1 Interval Overlap

Single pair comparison:

```text
O(1)
```

### 10.2 Guard Check

For `k` confirmed intervals in one daily guard:

```text
O(k)
```

Because one resource/day is bounded by practical slot count and duration policy, linear scanning may be acceptable initially.

If `k` becomes large, options include:

- Keep intervals sorted and use binary search.
- Use an interval tree.
- Use fixed slot representation only if product semantics become slot-based.

Do not add interval-tree complexity without benchmark evidence.

### 10.3 Candidate Filtering

For `n` candidate resources and `m` unavailable IDs:

- Build unavailable hash set: `O(m)`
- Filter candidates: `O(n)`
- Total: `O(n + m)`

### 10.4 Sorting

Sorting `n` candidates:

```text
O(n log n)
```

Prefer database ordering and bounded candidate sets when possible.

---

## 11. Couchbase Performance

### 11.1 Direct KV Access

Use direct key-value operations for:

- Organization by ID.
- Resource by ID.
- Reservation by ID.
- Idempotency by scoped key.
- Schedule guard by deterministic key.

Benefits:

- Predictable latency.
- No index visibility delay.
- Lower query overhead.

### 11.2 N1QL Usage

Use N1QL for:

- Resource search.
- User reservation lists.
- Pending approval lists.
- Reporting-oriented queries.
- Outbox polling if not using another mechanism.

Every critical query requires:

- Parameterization.
- Bounded result count.
- Appropriate index.
- `EXPLAIN` review.
- Representative dataset validation.

### 11.3 Index Design

Indexes should:

- Begin with tenant scope where appropriate.
- Match equality predicates before range predicates.
- Avoid excessive broad indexes.
- Be tested for write amplification.
- Be reviewed when query shapes change.

### 11.4 Scan Consistency

Avoid stronger scan consistency unless the use case requires it.

- Resource search may tolerate bounded staleness.
- Direct reservation retrieval uses KV.
- Correctness does not depend on eventually visible N1QL results.
- Schedule guards provide authoritative transaction coordination.

---

## 12. Transaction Performance

Couchbase transactions are used only where atomicity is required.

Guidelines:

- Keep transaction scope small.
- Avoid Redis, Kafka, HTTP, logging backends, or other network calls inside transactions.
- Load non-critical read-only metadata before the transaction where safe.
- Revalidate critical values inside the transaction if correctness requires it.
- Touch guard keys in deterministic order.
- Bound transaction timeout.
- Keep retry count small.
- Record transaction attempt count.

Transaction latency should be benchmarked for:

- Same-day reservation.
- Cross-midnight reservation.
- Cancellation.
- Extension.
- Approval.
- Hot-resource contention.

---

## 13. Schedule Guard Performance

Daily guard documents reduce contention compared with one lifetime resource schedule.

Monitor:

- Guard document size.
- Intervals per guard.
- CAS conflict rate.
- Transaction retry rate.
- Hot-resource distribution.

Potential optimization triggers:

| Signal | Possible Action |
|---|---|
| Guard exceeds size threshold | Smaller time buckets |
| Linear scan dominates | Sorted intervals/binary search |
| High retry rate | Backoff tuning or queue/fairness design |
| One resource dominates traffic | Dedicated hot-resource strategy |
| Multi-day duration introduced | Revisit bucket model |

---

## 14. Redis Performance

Redis operations require short deadlines.

Recommended behavior:

- Timeout quickly.
- Treat timeout as cache miss.
- Avoid retrying Redis multiple times in one request.
- Use pipelining only when measurements justify it.
- Add TTL jitter.
- Keep values compact.
- Avoid caching very large search responses.
- Monitor serialization cost separately from network latency.

Cache hit rate is useful only if it improves end-to-end latency and database load.

---

## 15. Kafka and Outbox Performance

### 15.1 Producer

Measure:

- Publish latency.
- Batch size.
- Compression impact.
- Retry count.
- Error rate.

### 15.2 Outbox Relay

Tune:

- Poll batch size.
- Poll interval.
- Number of workers.
- Claim lease.
- Retry backoff.
- Kafka producer batching.

### 15.3 Consumer

Measure:

- Processing latency.
- Consumer lag.
- Side-effect latency.
- Duplicate rate.
- Retry and DLQ rate.

Reservation API latency should remain independent of normal consumer processing.

---

## 16. Drogon and C++ Runtime

### 16.1 Request Handling

- Avoid blocking work on event-loop threads.
- Use appropriate async APIs or worker execution for blocking SDK calls.
- Bound request body size.
- Reuse thread-safe clients.
- Avoid per-request expensive object initialization.

### 16.2 Memory

- Prefer value semantics for small domain objects.
- Avoid unnecessary JSON copies.
- Move response payloads when ownership transfers.
- Use `std::string_view` only when lifetime is unambiguous.
- Avoid persistent per-request allocations.
- Profile before introducing custom allocators.

### 16.3 Serialization

Benchmark:

- Request DTO parsing.
- Reservation response serialization.
- Event envelope serialization.
- Couchbase document mapping.

---

## 17. Connection Pools and Concurrency Limits

Each external dependency requires deliberate limits:

- Couchbase connection and operation concurrency.
- Redis connection pool.
- Kafka producer buffer.
- HTTP in-flight request cap.
- Consumer worker count.
- Outbox relay worker count.

Unlimited concurrency can convert traffic spikes into dependency collapse.

Use backpressure:

- Bounded queues.
- Request rejection.
- Rate limiting.
- Timeouts.
- Circuit breakers.

---

## 18. Timeout Budget

Timeouts must align with the request deadline.

Example create reservation:

```text
Client timeout: 2 seconds
Server request deadline: 1.5 seconds
Couchbase transaction budget: 800 ms
Individual Couchbase operation: lower bounded timeout
Redis: 10–20 ms
```

Exact values are environment-dependent.

Rules:

- Inner timeout < outer timeout.
- Retries share one total deadline.
- Do not start work that cannot complete within remaining budget.
- Emit timeout category and dependency.

---

## 19. Load-Shedding Strategy

When saturated:

1. Protect correctness-critical writes.
2. Reject excess requests using `429` or `503`.
3. Keep health endpoints responsive.
4. Deprioritize optional operations.
5. Temporarily bypass unhealthy cache.
6. Avoid unbounded internal queues.
7. Preserve actionable retry guidance.

Haven does not silently queue reservation commands during overload in MVP.

---

## 20. Benchmark Types

### 20.1 Microbenchmarks

Use Google Benchmark for:

- `TimeInterval::overlaps`.
- Guard interval scan.
- Candidate filtering.
- Domain serialization helpers.
- Identifier parsing.
- Event envelope serialization.

Microbenchmarks do not represent API latency.

### 20.2 Integration Benchmarks

Measure:

- Couchbase direct KV.
- Resource N1QL search.
- Overlap query.
- Transaction create.
- Redis get/set.
- Kafka publish.
- Outbox polling.

### 20.3 Load Tests

Measure full APIs with:

- Constant arrival rate.
- Ramp-up.
- Spike.
- Soak.
- Hot-resource contention.
- Dependency degradation.

### 20.4 Concurrency Stress

Focus on correctness and retry amplification, not only throughput.

---

## 21. Benchmark Environment

Every result records:

- Git commit.
- Build type.
- Compiler and version.
- Dependency versions.
- CPU and memory.
- Operating system.
- Container limits.
- Couchbase configuration.
- Redis configuration.
- Kafka configuration.
- Dataset size.
- Traffic profile.
- Warm-up period.
- Duration.
- Concurrency.
- Error rate.

Results without environment metadata are not comparable.

---

## 22. Load-Test Scenarios

### Scenario A — Resource Search Baseline

- 50,000 resources.
- 5 million reservations.
- Mixed filters.
- 500 requests/sec.
- 10-minute steady state.

### Scenario B — Reservation Create Baseline

- Random low-contention resources.
- 100 creates/sec.
- Valid and conflicting requests.
- Idempotency retries included.

### Scenario C — Hot Resource

- 100 concurrent requests.
- Same resource and interval.
- Expect one success.
- Measure conflict latency and transaction retries.

### Scenario D — Cross-Midnight

- Reservations touch two daily guards.
- Measure transaction latency.

### Scenario E — Redis Failure

- Disable Redis under search load.
- Verify correctness and latency degradation.

### Scenario F — Kafka Failure

- Stop Kafka.
- Verify API stability and outbox backlog growth.

### Scenario G — Soak

- Several hours.
- Mixed read/write traffic.
- Monitor memory, connections, backlog, and latency drift.

---

## 23. Performance Regression Policy

A change requires investigation when:

- p95 latency regresses materially.
- Throughput falls materially.
- Memory per request increases significantly.
- Couchbase query plan changes unexpectedly.
- Transaction retries increase.
- Redis hit rate improves but end-to-end latency worsens.
- Consumer lag grows under the same workload.

Thresholds should account for benchmark noise.

Stable dedicated environments may fail CI on regressions. Noisy shared runners should report trends rather than enforce brittle limits.

---

## 24. Profiling Tools

Potential tools:

- Google Benchmark.
- `perf`.
- Flame graphs.
- Heap profiling.
- AddressSanitizer.
- ThreadSanitizer.
- Couchbase query profiler and `EXPLAIN`.
- Redis latency diagnostics.
- Kafka consumer lag tools.
- OpenTelemetry traces.
- Load tools such as k6, wrk, or Gatling-compatible alternatives.

Tool selection must match the question being investigated.

---

## 25. Capacity Planning

Capacity planning uses:

```text
peak request rate
× average dependency operations per request
× retry amplification
× safety margin
```

Example create path may generate:

- Resource read.
- Policy read/cache.
- Transaction with guard and reservation documents.
- Idempotency operation.
- Outbox operation.

Hot-resource retries increase write amplification and must be included.

---

## 26. Horizontal Scaling

Application instances are stateless and can scale horizontally.

Scaling constraints move to shared systems:

- Couchbase transaction throughput.
- Hot guard documents.
- Redis connection capacity.
- Kafka partitions.
- Outbox relay claim contention.

Adding application instances does not solve a single hot-resource serialization bottleneck.

---

## 27. Performance Risks

| Risk | Impact | Mitigation |
|---|---|---|
| Broad search | High latency | Mandatory type, filters, pagination |
| Offset pagination | Deep-page cost | Move to cursor if measured |
| Hot guard | Retry amplification | Metrics, backoff, future strategy |
| N1QL index lag | Stale search | Correctness remains in transaction |
| Large outbox backlog | Delayed events | Scale relay, alert on age |
| Redis timeout | Tail latency | Short timeout, circuit bypass |
| Excessive JSON mapping | CPU/allocation | Profile, reduce copies |
| Unbounded queues | Memory collapse | Backpressure |
| Large metric cardinality | Telemetry cost | Low-cardinality labels |

---

## 28. Optimization Decision Template

Before optimization, document:

1. Observed symptom.
2. Measured bottleneck.
3. Reproduction workload.
4. Baseline metrics.
5. Proposed change.
6. Correctness risk.
7. Complexity cost.
8. Expected improvement.
9. Post-change result.
10. Rollback condition.

Significant architectural optimization requires an ADR.

---

## 29. Test and Benchmark Repository Structure

```text
benchmarks/
├── domain/
│   ├── time_interval_benchmark.cpp
│   └── guard_scan_benchmark.cpp
├── infrastructure/
│   ├── couchbase_benchmark.cpp
│   ├── redis_benchmark.cpp
│   └── kafka_benchmark.cpp
└── application/
    └── candidate_filter_benchmark.cpp

tests/
└── performance/
    ├── scenarios/
    ├── datasets/
    └── reports/
```

---

## 30. Interview Discussion Notes

### How would you optimize availability search?

First inspect candidate count, query plan, overlap query, and serialization. Then improve indexes, bound filters, cache stable metadata, and consider cursor pagination or a read projection only after measurement.

### Why can horizontal scaling fail for a hot resource?

All conflicting reservations for one resource must serialize at the allocation boundary. More application instances can increase contention rather than eliminate it.

### Why separate p95 and p99?

Tail latency reveals retries, dependency stalls, connection pressure, and contention hidden by averages.

### Why not optimize domain objects immediately?

Most request latency is likely network and database time. Profiling determines whether C++ object allocation is material.

### How do you preserve correctness under load?

Use bounded queues, safe rejection, transactional allocation, and no bypass of conflict checks.

---

## 31. Review Checklist

- Are workload assumptions explicit?
- Are percentile targets defined?
- Is the dataset representative?
- Is hot-resource contention included?
- Are retry counts measured?
- Are query plans reviewed?
- Are benchmark environments recorded?
- Is correctness asserted during load?
- Is cache benefit measured end-to-end?
- Is the optimization simpler than its maintenance cost?

---

## 32. Summary

Haven's performance design prioritizes correctness, bounded work, representative benchmarking, percentile latency, and visibility into contention and retry amplification.

The primary likely bottlenecks are Couchbase queries and transactions, hot schedule guards, search candidate volume, and external dependency latency—not isolated C++ syntax.

---

## 33. Next Document

The next document is:

```text
docs/14-deployment.md
```

It defines local Docker Compose setup, application configuration, production topology, readiness, graceful shutdown, CI/CD flow, rollout, recovery, and operational runbooks.

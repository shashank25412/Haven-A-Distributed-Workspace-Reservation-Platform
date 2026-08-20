<div align="center">

# Haven — Distributed Workspace Reservation Platform

### A production-grade, multi-tenant workspace reservation backend built with Modern C++

Haven lets organizations discover and reserve shared resources — meeting rooms, desks, parking
slots, and other bookable assets — with strict correctness guarantees: no double bookings, no
duplicate commands, and no lost events.

<p>
  <img alt="C++20" src="https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white">
  <img alt="CMake" src="https://img.shields.io/badge/Build-CMake-064F8C?logo=cmake&logoColor=white">
  <img alt="Docker" src="https://img.shields.io/badge/Runtime-Docker_Compose-2496ED?logo=docker&logoColor=white">
  <img alt="Couchbase" src="https://img.shields.io/badge/Database-Couchbase-EA2328?logo=couchbase&logoColor=white">
  <img alt="Kafka" src="https://img.shields.io/badge/Events-Apache_Kafka-231F20?logo=apachekafka&logoColor=white">
</p>

**[Haven UI →](https://github.com/shashank25412/Haven-UI)**

</div>

---

## Overview

Haven is a **modular monolith**: a single deployable service with strict internal module
boundaries (presentation → application → domain, with infrastructure implementing application
contracts). This keeps operations simple while preserving clean seams for future extraction into
independent services if scale or team ownership requires it.

Reservation systems look simple until concurrent requests target the same resource. Haven treats
correctness — not just availability — as a first-class requirement: concurrent writes, safe
retries, reliable event delivery, and tenant isolation are all handled explicitly rather than left
as edge cases.

## Key Features

- **Reservations** — search active resources by type, location, capacity, and features; create,
  approve/reject, cancel, and extend reservations through well-defined state transitions.
- **Correctness** — transactional, per-resource schedule guards prevent overlapping confirmed
  reservations using half-open intervals `[startTime, endTime)`; idempotency keys scoped by
  tenant, caller, and operation deduplicate retries.
- **Reliability** — reservation state and outbox events commit in the same Couchbase transaction;
  a relay publishes to Kafka at-least-once; consumers deduplicate by `eventId`.
- **Security** — JWT-based authentication, application-layer authorization, and multi-tenant
  repository contracts that never leak cross-organization data.
- **Operability** — structured logging, metrics, tracing, liveness/readiness checks, bounded
  retries with backoff, and an optional Redis cache that never sits on the correctness path.

## Architecture

```text
Client → Drogon REST API → Application Use Cases → Domain Policies
                                    │
                                    ├─► Couchbase (resources, reservations,
                                    │    schedule guards, idempotency, outbox)
                                    └─► Redis (cache only — never authoritative)

Couchbase Outbox → Outbox Relay → Kafka → Notification / Reporting Consumers
```

| Principle             | Haven's approach                                                       |
| --------------------- | ---------------------------------------------------------------------- |
| Source of truth       | Couchbase                                                              |
| Availability          | Derived from active resources minus overlapping confirmed reservations |
| Concurrency control   | Optimistic transactions with per-resource daily schedule guards        |
| Command deduplication | Scoped idempotency records with payload hashes                         |
| Event reliability     | Transactional outbox + at-least-once Kafka delivery                    |
| Tenant isolation      | Trusted caller context propagated through every repository call        |
| Failure handling      | Typed errors, bounded retries, backoff, DLQs, observable degradation   |

Detailed diagrams live in [`docs/diagrams`](docs/diagrams); design docs and ADRs live in
[`docs`](docs) and [`docs/15-architecture-decisions`](docs/15-architecture-decisions).

## Technology Stack

| Area              | Technology                                                           |
| ----------------- | -------------------------------------------------------------------- |
| Language          | Modern C++20                                                         |
| HTTP framework    | Drogon                                                               |
| Build system      | CMake                                                                |
| Primary datastore | Couchbase                                                            |
| Cache             | Redis (optional, non-authoritative)                                  |
| Event streaming   | Apache Kafka                                                         |
| Local environment | Docker Compose                                                       |
| API contract      | OpenAPI / Swagger ([`api/haven-api-v1.yaml`](api/haven-api-v1.yaml)) |
| Testing           | CTest (unit, integration, concurrency)                               |
| Observability     | Structured logging, metrics, distributed tracing                     |

## API Overview

Full contract: [`docs/05-api-design.md`](docs/05-api-design.md).

| Method | Endpoint                                                               | Purpose                                  |
| ------ | ---------------------------------------------------------------------- | ---------------------------------------- |
| `GET`  | `/api/v1/organizations/{organizationId}/resources/{resourceId}`        | Read a tenant-scoped resource            |
| `GET`  | `/v1/resources/search`                                                 | Search resources and derive availability |
| `PUT`  | `/v1/reservations`                                                     | Create an idempotent reservation         |
| `POST` | `/v1/reservations/{id}/approve` \| `/reject` \| `/cancel` \| `/extend` | Reservation lifecycle transitions        |
| `GET`  | `/v1/reservations/{id}`                                                | Read a tenant-scoped reservation         |
| `GET`  | `/health/live` \| `/health/ready`                                      | Liveness / readiness                     |

```bash
curl --request PUT \
  --url http://localhost:8080/v1/reservations \
  --header "Authorization: Bearer <access-token>" \
  --header "Content-Type: application/json" \
  --header "Idempotency-Key: 8ce56da3-2db0-48ab-b054-70a78f3df986" \
  --data '{
    "resourceId": "room-101",
    "startTime": "2026-08-01T10:00:00Z",
    "endTime": "2026-08-01T11:00:00Z",
    "creator": {"name": "Shashank", "email": "shashank@example.com", "phone": "+91-9000000000"},
    "purpose": "Architecture review"
  }'
```

## Repository Structure

```text
Haven/
├── api/            OpenAPI contract
├── docs/           Design docs, ADRs, diagrams
├── include/haven/  Public headers (presentation, application, domain, infrastructure)
├── src/            Implementation, mirroring include/haven
├── tests/          Unit, integration, concurrency tests
├── deploy/         Couchbase provisioning and seed data
├── CMakeLists.txt
├── docker-compose.yml
├── README.md
└── SETUP_GUIDE.md
```

Dependency direction is strictly inward — `Presentation → Application → Domain`, with
`Infrastructure` implementing application-defined contracts. The domain layer has no dependency on
Drogon, Couchbase, Redis, Kafka, or JSON serialization.

## Getting Started

See [`SETUP_GUIDE.md`](SETUP_GUIDE.md) for prerequisites, builds, configuration, and testing. The
shortest path:

```bash
./scripts/bootstrap-vcpkg.sh
brew tap couchbaselabs/homebrew-couchbase && brew install couchbase-cxx-client
export CMAKE_PREFIX_PATH="$(brew --prefix couchbase-cxx-client)"
cmake --preset dev && cmake --build --preset dev
cp .env.example .env
docker compose up --build --detach
set -a; source .env; set +a
ctest --preset dev --output-on-failure
./build/dev/apps/server/haven-server
```

The companion **[Haven UI](../haven_ui)** provides the browser-based client for search, booking,
and approval workflows against this API.

Couchbase data is browsable at `http://localhost:8091`, bucket `haven`, scope `reservation`.

## Failure Behaviour

| Failure                      | Expected behaviour                                                 |
| ---------------------------- | ------------------------------------------------------------------ |
| Redis unavailable            | Bypass cache and continue through Couchbase                        |
| Kafka unavailable            | Commit reservation and retain event in the outbox                  |
| Couchbase unavailable        | Fail authoritative operations safely and mark the instance unready |
| Duplicate client request     | Return the original stored result                                  |
| Concurrent overlapping write | One transaction wins; the other returns a conflict                 |
| Consumer crash               | Kafka redelivers; consumer deduplicates by `eventId`               |
| Poison event                 | Retry with limits, then route to a consumer-specific DLQ           |

## Design Trade-offs

Haven deliberately avoids premature complexity: no microservices before scaling or ownership
requires them, no separate availability projection, no distributed lock for reservation safety, no
synchronous Kafka publishing inside business transactions, and no distributed transaction spanning
Couchbase and Kafka. These constraints keep the system explainable, testable, and operable.

## Contributing

1. Follow [`SETUP_GUIDE.md`](SETUP_GUIDE.md).
2. Review the relevant design docs and ADRs under [`docs`](docs).
3. Keep domain logic independent of frameworks.
4. Add tests for all changed behaviour; update docs when a design decision changes.

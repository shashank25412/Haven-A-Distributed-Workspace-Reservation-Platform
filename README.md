<div align="center">

# Haven-A-Distributed-Workspace-Reservation-Platform

### A production-minded, multi-tenant workspace reservation backend built with Modern C++

Haven is a backend platform for discovering and reserving shared resources such as meeting rooms, office desks, parking slots, and other bookable assets—without double bookings, duplicate commands, or unreliable event delivery.

<p>
  <img alt="C++20" src="https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white">
  <img alt="CMake" src="https://img.shields.io/badge/Build-CMake-064F8C?logo=cmake&logoColor=white">
  <img alt="Docker" src="https://img.shields.io/badge/Runtime-Docker_Compose-2496ED?logo=docker&logoColor=white">
  <img alt="Couchbase" src="https://img.shields.io/badge/Database-Couchbase-EA2328?logo=couchbase&logoColor=white">
  <img alt="Kafka" src="https://img.shields.io/badge/Events-Apache_Kafka-231F20?logo=apachekafka&logoColor=white">
</p>

</div>

---

## Overview

Haven is a portfolio-grade backend system designed to demonstrate the engineering decisions expected in a production reservation platform:

- concurrency-safe reservation workflows;
- multi-tenant data isolation;
- idempotent write APIs;
- approval workflows for priority resources;
- reliable event delivery using a transactional outbox;
- derived availability instead of duplicated state;
- observable, testable, and containerized infrastructure.

The project is intentionally structured as a **modular monolith**. It keeps the first production version operationally simple while preserving clear boundaries that can later be extracted into services when scale or team ownership justifies it.

> **Development status:** the architecture, requirements, ADRs, API design, data model, and operational design are established. Implementation is proceeding phase by phase.

---

## The Problem

Reservation systems look simple until multiple users try to reserve the same resource concurrently.

A production-quality solution must answer questions such as:

- What happens when two requests attempt to reserve the same room at the same time?
- Can a client safely retry after a timeout?
- How is availability calculated without becoming stale?
- What happens when Kafka is unavailable after a reservation is committed?
- How are organizations prevented from reading or modifying each other's data?
- Can an approval succeed if the resource became unavailable while the request was pending?

Haven treats these as core consistency and reliability problems—not edge cases.

---

## Key Features

### Reservation workflows

- Search active resources by organization, type, location, capacity, and features.
- Create reservations for fixed start and end times.
- Auto-confirm normal resources when the slot is free.
- Route priority resources through an approval workflow.
- Cancel or extend existing reservations through legal state transitions.
- Derive calendar and availability views from authoritative reservation data.

### Correctness and reliability

- Prevent overlapping confirmed reservations using a transactional schedule guard.
- Use half-open intervals: `[startTime, endTime)`.
- Scope idempotency records by tenant, caller, operation, and idempotency key.
- Persist reservation state and outbox events in the same Couchbase transaction.
- Publish Kafka events asynchronously with at-least-once delivery.
- Deduplicate consumer processing by `eventId`.
- Keep Redis outside the correctness boundary.

### Production engineering

- JWT authentication and application-layer authorization.
- Multi-tenant repository contracts.
- Structured logs, metrics, traces, health checks, and readiness checks.
- Bounded retries with backoff and failure classification.
- Docker Compose local environment.
- Unit, integration, concurrency, contract, and end-to-end test strategy.

---

## Architecture

```text
Client
  │
  ▼
Drogon REST API
  │
  ▼
Application Use Cases
  │
  ├──────────────► Domain Policies
  │
  ├──────────────► Couchbase
  │                 ├─ resources
  │                 ├─ reservations
  │                 ├─ schedule guards
  │                 ├─ idempotency records
  │                 └─ transactional outbox
  │
  └──────────────► Redis
                    └─ cache / rate limiting only

Couchbase Outbox
  │
  ▼
Outbox Relay ──► Kafka ──► Notification Consumer
                       └──► Reporting / Audit Consumer
```

### Architectural principles

| Principle | Haven's approach |
|---|---|
| Deployment model | Modular monolith with explicit module boundaries |
| Source of truth | Couchbase |
| Availability | Derived from resources and overlapping confirmed reservations |
| Concurrency control | Optimistic transactions and per-resource daily schedule guards |
| Command deduplication | Scoped idempotency records with payload hashes |
| Event reliability | Transactional outbox and at-least-once Kafka delivery |
| Cache role | Performance optimization only |
| Tenant isolation | Trusted caller context propagated into every repository operation |
| Failure handling | Typed errors, bounded retries, backoff, DLQs, and observable degradation |

Detailed diagrams are available in [`docs/diagrams`](docs/diagrams):

- [`architecture.drawio`](docs/diagrams/architecture.drawio)
- [`sequence.drawio`](docs/diagrams/sequence.drawio)
- [`class.drawio`](docs/diagrams/class.drawio)
- [`deployment.drawio`](docs/diagrams/deployment.drawio)
- [`state.drawio`](docs/diagrams/state.drawio)

---

## Core Consistency Model

### Availability is derived

Haven does not maintain a separate authoritative availability document.

For a requested interval:

```text
available resources
    = active resources matching the search filters
    - resources with overlapping CONFIRMED reservations
```

Two intervals overlap when:

```text
existing.start < requested.end
&&
existing.end > requested.start
```

Search results are advisory. Reservation creation, approval, and extension always revalidate the slot inside the authoritative write transaction.

### Double-booking prevention

Each resource has a date-partitioned schedule guard containing its confirmed intervals.

A write transaction:

1. loads the relevant guard document;
2. checks for interval overlap;
3. updates the guard;
4. persists the reservation;
5. persists the idempotency result;
6. persists the outbox event;
7. commits all changes atomically.

If two transactions race, only one can commit the expected guard version. The losing transaction reloads the latest state and either retries safely or returns a business conflict.

### Transactional outbox

Haven never publishes to Kafka inside the reservation transaction.

Instead, the transaction stores an outbox event alongside the reservation. A relay later:

1. claims pending events using optimistic concurrency;
2. publishes them to Kafka;
3. records the broker acknowledgment;
4. marks the outbox event as published.

A relay crash can cause duplicate delivery, but it cannot silently lose the event. Consumers therefore process events idempotently using `eventId`.

---

## Reservation Lifecycle

```text
                         approve + recheck
PENDING_APPROVAL  ─────────────────────────►  CONFIRMED
       │                                         │
       ├── reject ──► REJECTED                   ├── cancel ──► CANCELLED
       └── cancel ──► CANCELLED                  ├── extend ──► CONFIRMED
                                                 ├── complete ─► COMPLETED
                                                 └── expire ───► EXPIRED
```

Pending approval does not reserve capacity in the MVP. Availability is checked again when an approver confirms the request.

---

## Technology Stack

| Area | Technology |
|---|---|
| Language | Modern C++20 |
| HTTP framework | Drogon |
| Build system | CMake |
| Primary datastore | Couchbase |
| Cache / rate limiting | Redis |
| Event streaming | Apache Kafka |
| Local environment | Docker Compose |
| API contract | OpenAPI / Swagger |
| Testing | CTest with unit and integration test targets |
| Observability | Structured logging, metrics, distributed tracing |
| Documentation | Markdown, ADRs, diagrams.net XML |

---

## API Overview

The exact contract is documented in [`docs/05-api-design.md`](docs/05-api-design.md).

| Method | Endpoint | Purpose |
|---|---|---|
| `GET` | `/v1/resources/search` | Search resources and derive availability |
| `PUT` | `/v1/reservations` | Create an idempotent reservation |
| `POST` | `/v1/reservations/{id}/approve` | Approve a pending reservation |
| `POST` | `/v1/reservations/{id}/reject` | Reject a pending reservation |
| `POST` | `/v1/reservations/{id}/cancel` | Cancel a reservation |
| `POST` | `/v1/reservations/{id}/extend` | Extend a confirmed reservation |
| `GET` | `/v1/reservations/{id}` | Read a tenant-scoped reservation |
| `GET` | `/health/live` | Process liveness |
| `GET` | `/health/ready` | Traffic readiness |

### Example create request

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
    "creator": {
      "name": "Shashank",
      "email": "shashank@example.com",
      "phone": "+91-9000000000"
    },
    "purpose": "Architecture review"
  }'
```

Example response:

```json
{
  "reservationId": "res_01JZ...",
  "resourceId": "room-101",
  "status": "CONFIRMED",
  "startTime": "2026-08-01T10:00:00Z",
  "endTime": "2026-08-01T11:00:00Z"
}
```

---

## Repository Structure

```text
Haven/
├── AI_CONTEXT/                 # Engineering rules and review context
├── docs/
│   ├── 00-introduction.md
│   ├── 01-requirements.md
│   ├── 02-high-level-design.md
│   ├── 03-low-level-design.md
│   ├── 04-domain-model.md
│   ├── 05-api-design.md
│   ├── 06-database-design.md
│   ├── 07-event-driven-design.md
│   ├── 08-concurrency.md
│   ├── 09-caching.md
│   ├── 10-security.md
│   ├── 11-observability.md
│   ├── 12-testing.md
│   ├── 13-performance.md
│   ├── 14-deployment.md
│   ├── adr/
│   └── diagrams/
├── include/haven/
│   ├── domain/
│   ├── application/
│   ├── infrastructure/
│   └── presentation/
├── src/
│   ├── domain/
│   ├── application/
│   ├── infrastructure/
│   └── presentation/
├── tests/
│   ├── unit/
│   ├── integration/
│   ├── concurrency/
│   └── e2e/
├── config/
├── docker/
├── CMakeLists.txt
├── docker-compose.yml
└── README.md
```

The dependency direction is intentionally inward:

```text
Presentation ─► Application ─► Domain
                     ▲
Infrastructure ──────┘
```

The domain layer does not depend on Drogon, Couchbase, Redis, Kafka, or JSON serialization.

---

## Local Development

### Prerequisites

- Docker and Docker Compose
- CMake 3.25+
- A C++20-compatible compiler
- Git

### Start infrastructure and the application

```bash
git clone <repository-url>
cd Haven

cp .env.example .env
docker compose up --build -d
```

Check the service:

```bash
curl http://localhost:8080/health/live
curl http://localhost:8080/health/ready
```

View logs:

```bash
docker compose logs -f haven-api
```

Stop the environment:

```bash
docker compose down
```

### Native build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

> Commands and service names may evolve while the implementation skeleton is being finalized. The Docker Compose file and CMake targets remain the source of truth.

---

## Configuration

Runtime configuration is supplied through environment variables or mounted configuration files.

Typical settings include:

```dotenv
HAVEN_HTTP_PORT=8080
HAVEN_LOG_LEVEL=info

COUCHBASE_CONNECTION_STRING=couchbase://couchbase
COUCHBASE_USERNAME=Administrator
COUCHBASE_PASSWORD=password
COUCHBASE_BUCKET=haven

REDIS_HOST=redis
REDIS_PORT=6379

KAFKA_BOOTSTRAP_SERVERS=kafka:9092
KAFKA_RESERVATION_TOPIC=haven.reservation.events.v1

JWT_ISSUER=https://identity.example.com/
JWT_AUDIENCE=haven-api
JWT_JWKS_URI=https://identity.example.com/.well-known/jwks.json
```

Secrets must never be committed to the repository. Production deployments should use a dedicated secret-management system.

---

## Testing Strategy

Haven uses multiple test layers because correctness cannot be demonstrated through unit tests alone.

### Unit tests

- interval-overlap rules;
- reservation state transitions;
- policy validation;
- idempotency payload matching;
- event serialization;
- error mapping.

### Integration tests

- Couchbase repositories and indexes;
- multi-document transactions;
- schedule-guard conflicts;
- Redis fallback behavior;
- Kafka publishing and consumer deduplication;
- OpenAPI contract validation.

### Concurrency tests

- multiple requests for the same resource and interval;
- approval racing with cancellation;
- extension racing with a new reservation;
- duplicate requests using the same idempotency key;
- relay crashes around Kafka acknowledgments.

### End-to-end tests

- search → create → retrieve;
- create → approve;
- create → cancel;
- create → extend;
- reservation event → notification/reporting consumer.

Run the test suite:

```bash
ctest --test-dir build --output-on-failure
```

---

## Observability

Every request and event-processing path carries correlation metadata.

Key signals include:

- HTTP latency and error rates;
- reservation conflicts;
- Couchbase transaction retries;
- Redis hits, misses, and timeouts;
- outbox backlog size and oldest-event age;
- Kafka publishing failures and consumer lag;
- duplicate consumer deliveries;
- DLQ growth;
- readiness and dependency health.

High-cardinality identifiers such as `reservationId`, `resourceId`, and `eventId` belong in structured logs and traces—not metric labels.

---

## Failure Behaviour

| Failure | Expected behaviour |
|---|---|
| Redis unavailable | Bypass cache and continue through Couchbase |
| Kafka unavailable | Commit reservation and retain event in the outbox |
| Couchbase unavailable | Fail authoritative operations safely and mark the instance unready |
| Duplicate client request | Return the original stored result |
| Concurrent overlapping write | One transaction wins; the other returns a conflict |
| Consumer crash | Kafka redelivers; consumer deduplicates by `eventId` |
| Poison event | Retry with limits, then route to a consumer-specific DLQ |
| Identity provider unavailable | Continue validating tokens with cached trusted keys while valid |

---

## Documentation

The design is documented before implementation so that code can be reviewed against explicit decisions rather than implicit assumptions.

Start here:

1. [`docs/00-introduction.md`](docs/00-introduction.md)
2. [`docs/01-requirements.md`](docs/01-requirements.md)
3. [`docs/02-high-level-design.md`](docs/02-high-level-design.md)
4. [`docs/03-low-level-design.md`](docs/03-low-level-design.md)
5. [`docs/04-domain-model.md`](docs/04-domain-model.md)

Important technical deep dives:

- [`docs/06-database-design.md`](docs/06-database-design.md)
- [`docs/07-event-driven-design.md`](docs/07-event-driven-design.md)
- [`docs/08-concurrency.md`](docs/08-concurrency.md)
- [`docs/10-security.md`](docs/10-security.md)
- [`docs/11-observability.md`](docs/11-observability.md)
- [`docs/12-testing.md`](docs/12-testing.md)

Architecture decisions are captured as ADRs, including the modular monolith, Couchbase, Drogon, optimistic locking, Kafka, Redis, JWT authentication, and transactional outbox choices.

---

## Engineering Roadmap

### Foundation

- [x] Requirements and domain boundaries
- [x] High-level and low-level design
- [x] API, persistence, concurrency, and event design
- [x] ADRs and architecture diagrams
- [ ] CMake project skeleton and dependency management
- [ ] Docker Compose development environment

### MVP

- [ ] Health and readiness endpoints
- [ ] Resource search API
- [ ] Idempotent reservation creation
- [ ] Schedule-guard conflict prevention
- [ ] Approval, cancellation, and extension workflows
- [ ] Transactional outbox relay
- [ ] Notification and reporting consumers
- [ ] OpenAPI documentation

### Production hardening

- [ ] Complete observability dashboards and alerts
- [ ] Load and soak testing
- [ ] Failure-injection tests
- [ ] Security review and threat model
- [ ] Backup and restore validation
- [ ] Deployment automation
- [ ] Performance tuning based on measured bottlenecks

---

## Design Trade-offs

Haven deliberately avoids several premature optimizations:

- no microservices before independent scaling or ownership requires them;
- no separate availability projection as an MVP correctness dependency;
- no Redis lock for reservation safety;
- no synchronous Kafka publishing inside business transactions;
- no recurring reservations in the first release;
- no distributed transaction across Couchbase and Kafka.

These constraints keep the system explainable, testable, and operationally realistic.

---

## Project Goals

Haven is built as both a useful system and an engineering case study. It is intended to demonstrate:

- production-grade Modern C++ backend design;
- clean boundaries and dependency inversion;
- practical concurrency control;
- event-driven reliability patterns;
- database-aware data modelling;
- operational thinking beyond the happy path;
- senior-level design communication through documentation and ADRs.

---

## Contributing

This project is currently developed as a focused portfolio system. Issues and design discussions are welcome.

Before contributing:

1. read the documents under `AI_CONTEXT/`;
2. review the relevant design documents and ADRs;
3. keep domain logic independent of frameworks;
4. add tests for all changed behaviour;
5. update documentation when a design decision changes.

---

# Development Setup

## Prerequisites

### macOS

Install the required command-line tools:

```bash
xcode-select --install
```

Install the build utilities:

```bash
brew install cmake ninja pkgconf
```

Optional developer tooling:

```bash
brew install llvm
```

Docker-based development requires Docker Desktop.

Verify the environment:

```bash
cmake --version
ninja --version
pkg-config --version
clang++ --version
docker version
```

Haven requires CMake 3.28 or newer.

### Repository path

Keep the repository in a tooling-safe path without semicolons or unusual punctuation.

Recommended:

```text
~/Documents/haven
```

Avoid paths such as:

```text
Haven; A Distributed Workspace Reservation Platform
```

CMake treats semicolons as list separators.

---

## Bootstrap vcpkg

Haven uses a repository-local, pinned vcpkg installation.

Run:

```bash
chmod +x scripts/bootstrap-vcpkg.sh
./scripts/bootstrap-vcpkg.sh
```

The script creates:

```text
.build-tools/
├── vcpkg/
└── vcpkg-downloads/
```

These directories are local development artifacts and must not be committed.

The script is safe to run more than once.

---

## Native Debug Build

Configure:

```bash
cmake --preset dev
```

Build:

```bash
cmake --build --preset dev
```

Run tests:

```bash
ctest --preset dev
```

Start Haven:

```bash
./build/dev/apps/server/haven-server
```

The process listens on:

```text
0.0.0.0:8080
```

unless overridden through environment variables.

---

## Sanitizer Build

Configure with AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
cmake --preset dev-asan
```

Build:

```bash
cmake --build --preset dev-asan
```

Run tests:

```bash
ctest --preset dev-asan
```

The sanitizer build also treats Haven compiler warnings as errors.

---

## Release Build

Configure:

```bash
cmake --preset release
```

Build:

```bash
cmake --build --preset release
```

Run tests:

```bash
ctest --preset release
```

---

# Runtime Configuration

Copy the example environment file:

```bash
cp .env.example .env
```

Currently supported variables:

| Variable             |   Default | Description           |
| -------------------- | --------: | --------------------- |
| `HAVEN_HTTP_ADDRESS` | `0.0.0.0` | HTTP listener address |
| `HAVEN_HTTP_PORT`    |    `8080` | HTTP listener port    |

The port must be an integer between `1` and `65535`.

Invalid configuration causes startup to fail with a non-zero exit status.

## Native environment loading

The native executable does not automatically parse `.env`.

Load the variables into the current shell:

```bash
set -a
source .env
set +a
```

Then run:

```bash
./build/dev/apps/server/haven-server
```

Docker Compose reads the root `.env` file automatically.

---

# Health Endpoint

## Liveness

```http
GET /health/live
```

Example:

```bash
curl --fail http://localhost:8080/health/live
```

Expected response:

```json
{
  "service": "haven-api",
  "status": "alive"
}
```

Liveness represents only the health of the Haven process.

It must not depend on:

* Couchbase
* Redis
* Kafka
* external network services

Dependency health will later be exposed through a separate readiness endpoint.

---

# Testing

Run all unit tests through CTest:

```bash
ctest --preset dev
```

Run the GoogleTest executable directly:

```bash
./build/dev/tests/unit/haven_unit_tests
```

Run tests matching a CTest name:

```bash
ctest --preset dev -R LiveResponse
```

Current tests intentionally avoid:

* external services
* Docker dependencies
* network ports
* Couchbase
* Redis
* Kafka

Infrastructure integration tests will be introduced separately.

---

# Formatting

Format one file:

```bash
clang-format -i apps/server/main.cpp
```

Format all C++ files:

```bash
find apps include src tests \
  \( -name '*.cpp' -o -name '*.hpp' \) \
  -print0 |
xargs -0 clang-format -i
```

Check formatting without changing files:

```bash
find apps include src tests \
  \( -name '*.cpp' -o -name '*.hpp' \) \
  -print0 |
xargs -0 clang-format --dry-run --Werror
```

When LLVM is installed through Homebrew, the executable may be located at:

```text
/opt/homebrew/opt/llvm/bin/clang-format
```

---

# Static Analysis

CMake generates:

```text
build/dev/compile_commands.json
```

after successful configuration.

Analyze one file:

```bash
clang-tidy apps/server/main.cpp -p build/dev
```

Analyze all Haven source and test files:

```bash
find apps src tests \
  -name '*.cpp' \
  -print0 |
xargs -0 -n1 clang-tidy -p build/dev
```

When LLVM is installed through Homebrew, the executable may be located at:

```text
/opt/homebrew/opt/llvm/bin/clang-tidy
```

---

# Docker

## Start Docker

On macOS, start Docker Desktop:

```bash
open -a Docker
```

Verify that the Docker daemon is available:

```bash
docker info
```

The command must succeed before building or starting containers.

## Build the image

```bash
docker buildx build \
  --load \
  --tag haven-api:foundation \
  .
```

## Run the image directly

```bash
docker run \
  --rm \
  --name haven-api \
  --publish 8080:8080 \
  haven-api:foundation
```

Verify:

```bash
curl --fail http://localhost:8080/health/live
```

## Run with Docker Compose

Start:

```bash
docker compose up --build --detach
```

Check service status:

```bash
docker compose ps
```

View logs:

```bash
docker compose logs --follow haven-api
```

Verify:

```bash
curl --fail http://localhost:8080/health/live
```

Stop:

```bash
docker compose down
```

The current Compose topology contains only:

```text
haven-api
```

Couchbase, Redis, and Kafka will be added when the corresponding infrastructure adapters exist.

---

# Troubleshooting

## CMake cannot find Ninja

Error:

```text
CMAKE_MAKE_PROGRAM is not set
```

Install and verify Ninja:

```bash
brew install ninja
ninja --version
```

For Apple Silicon Homebrew:

```bash
eval "$(/opt/homebrew/bin/brew shellenv)"
```

## vcpkg cannot find `pkg-config`

Install:

```bash
brew install pkgconf
```

Verify:

```bash
pkg-config --version
```

## vcpkg reports an HTTP/2 framing error

Example:

```text
curl operation failed with error code 16
```

This is a network transport failure while downloading dependency source archives.

The repository uses:

```text
.build-tools/vcpkg-downloads/
```

as its vcpkg download cache.

Verify that the directory exists:

```bash
mkdir -p .build-tools/vcpkg-downloads
```

Also check for unexpected proxy variables:

```bash
env | grep -i proxy
```

When no proxy is intentionally configured:

```bash
unset HTTP_PROXY HTTPS_PROXY ALL_PROXY
unset http_proxy https_proxy all_proxy
```

## Docker cannot connect to the daemon

Error:

```text
failed to connect to the docker API
```

Start Docker Desktop:

```bash
open -a Docker
```

Then verify:

```bash
docker info
```

## Build directory contains stale paths

After renaming or moving the repository:

```bash
rm -rf build
cmake --preset dev
```

CMake caches absolute paths, so an existing build directory should not be reused after moving the project.

---

# Current Development Phase

Haven is currently inside:

```text
Phase 3–5:
Environment setup, project skeleton, and first APIs
```

Current progress:

```text
Phase 3 — Environment setup     In progress
Phase 4 — Project skeleton      In progress
Phase 5 — First APIs            Liveness started
```

The liveness endpoint is treated as a foundation-level API because it proves that the process, HTTP framework, routing, tests, and container runtime are connected correctly.

---

# Next Milestone

After the Foundation milestone is verified, the next work will introduce:

1. validated bootstrap configuration;
2. structured startup logging;
3. graceful shutdown verification;
4. readiness semantics;
5. Couchbase connectivity;
6. integration-test infrastructure.

Business reservation logic will not be added until the runtime foundation is stable.

---

# Documentation Policy

Haven documents:

* public contracts;
* invariants;
* ownership and lifetime expectations;
* failure behavior;
* thread-safety guarantees;
* architectural intent.

Comments should explain **why**, not narrate obvious code.

Broader decisions remain in:

```text
docs/
docs/adr/
AI_CONTEXT/
```

HTTP contracts will be documented through OpenAPI as APIs are implemented.

---

## License

A license will be added before the first public release.

---

<div align="center">

**Haven — reserve shared resources with confidence.**

</div>
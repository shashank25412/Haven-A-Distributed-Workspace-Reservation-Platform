---
title: ADR-002 - Use Couchbase as the Primary Datastore
status: Accepted
date: 2026-07-20
decision_owners:
  - Shashank Kumar
reviewers:
  - Kumar Rahul
supersedes: []
superseded_by: null
related_documents:
  - ../01-requirements.md
  - ../02-high-level-design.md
  - ../04-domain-model.md
  - ../06-database-design.md
  - ../08-concurrency.md
  - ../13-performance.md
  - ../14-deployment.md
---

# ADR-002: Use Couchbase as the Primary Datastore

## 1. Status

**Accepted**

---

## 2. Context

Haven requires a persistent datastore for:

- Organizations and tenant policies
- Resource metadata
- Reservations
- Idempotency records
- Concurrency guard documents
- Event outbox records
- Consumer deduplication records

The datastore must support:

- Tenant-scoped reads and writes
- Direct lookup by opaque identifier
- JSON-oriented documents
- Secondary indexes
- Time-interval overlap queries
- Optimistic concurrency
- Atomic multi-document workflows
- Horizontal scaling
- High read throughput
- Persistent reservation history
- Local Docker-based development

The reservation platform must prevent conflicting confirmed reservations even when multiple requests execute concurrently.

The datastore choice therefore affects:

- Domain persistence boundaries
- Query design
- Concurrency control
- Event publication reliability
- Idempotency
- Deployment complexity
- Portfolio learning objectives

The main alternatives evaluated were:

- Couchbase
- PostgreSQL
- MongoDB
- DynamoDB-style key-value/document storage

---

## 3. Problem Statement

Which primary datastore should Haven use for the MVP and early production-oriented design?

The selected datastore must support the project requirements while keeping the design defensible, testable, and operationally understandable.

---

## 4. Decision Drivers

| Priority | Driver | Importance |
|---:|---|---|
| 1 | Reservation correctness under concurrency | Critical |
| 2 | Tenant-scoped query support | Critical |
| 3 | Direct key-value access | High |
| 4 | JSON document model | High |
| 5 | Optimistic concurrency support | High |
| 6 | Multi-document atomicity | High |
| 7 | Secondary indexing and time-range queries | High |
| 8 | Horizontal scalability | Medium |
| 9 | Local development experience | Medium |
| 10 | Operational maturity | Medium |
| 11 | Learning value for the project owner | Medium |
| 12 | Portability to another datastore | Medium |

---

## 5. Options Considered

### Option A — Couchbase

A distributed document and key-value database supporting:

- JSON documents
- Direct key-value operations
- N1QL/SQL++ queries
- Secondary indexes
- CAS-based optimistic concurrency
- Transactions
- Buckets, scopes, and collections
- Clustering and replication

### Option B — PostgreSQL

A relational database supporting:

- ACID transactions
- Rich SQL
- Strong consistency
- Constraints
- Range types
- Exclusion constraints
- Mature tooling
- Relational joins
- Extensive operational ecosystem

### Option C — MongoDB

A document database supporting:

- JSON/BSON documents
- Secondary indexes
- Transactions
- Change streams
- Broad ecosystem
- Flexible schema

### Option D — DynamoDB-Style Storage

A key-value/document model supporting:

- High horizontal scalability
- Conditional writes
- Predictable key access
- Managed operations in cloud deployments
- Limited ad hoc query flexibility compared with SQL databases

---

## 6. Evaluation

| Criteria | Couchbase | PostgreSQL | MongoDB | DynamoDB-Style |
|---|---|---|---|---|
| Direct KV access | Excellent | Good via indexed lookup | Good | Excellent |
| JSON document model | Excellent | Good with JSONB | Excellent | Excellent |
| SQL-like querying | Strong | Excellent | Moderate | Limited |
| Time-overlap queries | Good with indexes | Excellent | Good | Requires careful modeling |
| Optimistic concurrency | CAS | Version column / locks | Version field / transactions | Conditional writes |
| Multi-document transactions | Supported | Excellent | Supported | Limited/model-dependent |
| Horizontal scaling | Strong | More operationally involved | Strong | Excellent |
| Local development | Good with Docker | Excellent | Excellent | Weaker without emulator |
| Operational maturity | Strong | Excellent | Strong | Strong in managed cloud |
| Portfolio learning value | High | High | Medium | High |
| Fit with selected project goal | Excellent | Excellent technically | Good | Medium |

---

## 7. Decision

Haven will use **Couchbase as the primary authoritative datastore**.

Couchbase will store:

- Organization documents
- Resource documents
- Reservation documents
- Idempotency documents
- Resource schedule guard documents
- Event outbox documents
- Consumer deduplication records where appropriate

Redis remains an optional cache and rate-limiting dependency.

Kafka remains the asynchronous event transport.

---

## 8. Rationale

### 8.1 The Project Intentionally Includes Couchbase

One explicit Haven goal is to gain production-oriented experience with Couchbase.

Selecting Couchbase is therefore not incidental. The project is designed to demonstrate:

- Document modeling from access patterns
- Direct key-value retrieval
- N1QL/SQL++ indexing
- CAS-based optimistic concurrency
- Multi-document transactions
- Tenant-aware document organization
- Operational bootstrap and index management

This learning objective is valid because Couchbase also satisfies the functional requirements.

### 8.2 Direct Key-Value Access Fits Core Lookups

Haven frequently retrieves known entities by ID:

- Organization
- Resource
- Reservation
- Idempotency record
- Schedule guard

Deterministic keys provide predictable direct access without requiring secondary-index visibility.

### 8.3 JSON Documents Fit Aggregate Persistence

Organization, Resource, and Reservation are naturally represented as documents with nested value objects.

Examples:

- Resource location
- Feature arrays
- Approval metadata
- Organization policy
- Event envelope payload

This avoids unnecessary relational decomposition for aggregate-local data.

### 8.4 CAS Supports Optimistic Concurrency

Couchbase CAS provides a native mechanism to detect stale updates to existing documents.

It is suitable for:

- Approval racing with cancellation
- Extension racing with another update
- Outbox relay claims
- Schedule guard updates
- Idempotency state transitions

### 8.5 Transactions Support Atomic Reservation Workflows

Reservation creation may require atomic changes to:

- Idempotency record
- Schedule guard documents
- Reservation document
- Outbox documents

Couchbase transactions provide a practical way to preserve consistency across these records.

### 8.6 N1QL Supports Operational Query Needs

Haven requires queries for:

- Resource search
- User reservation history
- Pending approvals
- Overlapping reservations
- Outbox polling

N1QL/SQL++ and secondary indexes support these access patterns without requiring every query to be encoded solely into document keys.

### 8.7 Couchbase Supports Horizontal Evolution

Couchbase provides a credible future path for:

- Larger datasets
- More tenants
- Higher read throughput
- Distributed clusters
- Replication

The MVP does not depend on that future scale, but the datastore does not create an obvious early ceiling.

---

## 9. Data Modeling Rules

### 9.1 Separate Collections

Recommended:

```text
Bucket: haven

Scopes:
  core
  integration

Collections:
  core.organizations
  core.resources
  core.reservations
  core.idempotency
  core.resource_schedules
  integration.outbox
  integration.consumer_dedup
```

### 9.2 Separate Aggregates

Resource documents must not embed reservation history.

Reasons:

- Unbounded growth
- Hot-document contention
- Different lifecycle
- Different query requirements
- Difficult history pagination

### 9.3 Tenant Identity

Every tenant-owned document contains `organizationId`.

Direct document keys also include tenant context where appropriate.

### 9.4 Schema Version

Every persisted document includes:

```json
{
  "schemaVersion": 1
}
```

### 9.5 Availability

Availability is not persisted as authoritative state.

It is derived from:

- Resource state
- Reservation data
- Schedule guard projection
- Requested interval

### 9.6 Guard Documents

Per-resource daily schedule guard documents are permitted as rebuildable concurrency projections.

They do not replace Reservation as the source of truth.

---

## 10. Key Conventions

| Document | Key Format |
|---|---|
| Organization | `org::<organizationId>` |
| Resource | `resource::<organizationId>::<resourceId>` |
| Reservation | `reservation::<organizationId>::<reservationId>` |
| Idempotency | `idem::<organizationId>::<userId>::<operation>::<keyHash>` |
| Schedule guard | `schedule::<organizationId>::<resourceId>::<utcDate>` |
| Outbox | `outbox::<organizationId>::<eventId>` |
| Consumer dedup | `consumer::<consumerName>::<eventId>` |

Key rules:

- Opaque IDs
- No secrets
- No unbounded raw input
- Stable documented format
- Tenant-aware direct access

---

## 11. Query Strategy

### 11.1 Direct KV Operations

Preferred for:

- Known organization ID
- Known resource ID
- Known reservation ID
- Idempotency key
- Schedule guard
- Known outbox event

### 11.2 N1QL Queries

Used for:

- Search resources
- Find user reservations
- Find pending approvals
- Find overlapping reservations
- Poll pending outbox events
- Administrative diagnosis

### 11.3 Query Consistency

Different use cases use different consistency levels.

- Search may tolerate bounded staleness.
- Known reservation retrieval should use direct key access.
- Conflict correctness must not rely only on index visibility.
- Schedule guards and transactions provide authoritative allocation coordination.

---

## 12. Concurrency Model

### 12.1 Existing Document Updates

Use CAS or transaction conflict detection.

No silent overwrite is allowed.

### 12.2 New Reservation Creation

A normal query-then-insert sequence is unsafe.

Haven will use:

- Couchbase transactions
- Resource schedule guard documents
- Idempotency record
- Reservation insert
- Outbox insert

as one atomic workflow where required.

### 12.3 Retry

Retry only classified transient failures.

Retries are:

- Bounded
- Deadline-aware
- Observable
- Jittered where appropriate

Business conflicts are not retried.

---

## 13. Transaction Boundaries

Candidate atomic create transaction:

```text
insert idempotency record
read/create affected schedule guards
check overlap
append interval to guards
insert reservation
insert outbox events
complete idempotency response
commit
```

Cancellation transaction:

```text
load reservation
load affected guards
remove interval
update reservation status
insert outbox event
commit
```

Approval transaction:

```text
load pending reservation
check guard overlap
append interval
update reservation to confirmed
insert outbox event
commit
```

No Kafka or Redis call occurs inside the Couchbase transaction.

---

## 14. Index Strategy

Critical indexes support:

- Tenant + resource type + active state
- Tenant + resource + blocking status + time
- Tenant + creator + date/status
- Tenant + pending approval status
- Outbox publication status + next attempt

Every critical index must be:

- Version-controlled
- Created through repeatable scripts
- Verified using `EXPLAIN`
- Tested with representative data
- Reviewed for write amplification

---

## 15. Operational Requirements

### 15.1 Local Development

Couchbase runs through Docker Compose.

Bootstrap scripts create:

- Bucket
- Scopes
- Collections
- Indexes
- Seed data

### 15.2 Production

Production requires:

- Clustered deployment
- Private networking
- TLS
- Scoped credentials
- Backups
- Restore tests
- Monitoring
- Capacity planning
- Controlled index changes

### 15.3 Readiness

Haven is not ready to serve authoritative traffic when Couchbase is unavailable.

---

## 16. Security Requirements

- Couchbase is not publicly exposed.
- Credentials are externalized.
- Least-privilege access is used.
- Queries are parameterized.
- Tenant scope is mandatory.
- Raw Couchbase errors do not reach clients.
- Administrative UI access is restricted.
- Backup access is restricted.
- TLS is used in production.

---

## 17. Consequences

### 17.1 Positive

- Natural JSON aggregate persistence
- Fast direct-key access
- Native CAS
- Transaction support
- Secondary indexes
- SQL-like querying
- Horizontal scaling path
- Strong portfolio learning value
- Suitable for multi-tenant document organization
- Good fit for outbox and idempotency documents

### 17.2 Negative

- Time-range and overlap modeling requires careful indexing.
- Transactions add latency and complexity.
- Couchbase operations and administration are less universally familiar than PostgreSQL.
- Guard documents introduce a derived consistency projection.
- Query performance depends heavily on index design.
- Local development is heavier than an embedded database.
- Migration and backup procedures must be learned.
- Some relational constraints must be enforced in application/domain logic.

### 17.3 Neutral

- Couchbase does not eliminate the need for Redis.
- Couchbase does not replace Kafka.
- A document database does not justify embedding unrelated aggregates.
- Schema flexibility does not mean schema discipline is optional.

---

## 18. Risks and Mitigations

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| Incorrect overlap query | Medium | Critical | Domain matrix tests and integration tests |
| Query scans due to bad index | Medium | High | `EXPLAIN`, benchmark, index review |
| Transaction retry amplification | Medium | High | Bounded retries and contention metrics |
| Hot guard document | Medium | High | Daily buckets and hot-resource monitoring |
| Index visibility misunderstood | Medium | Critical | Direct KV/guard correctness path |
| Schema drift | Medium | Medium | Schema version and mapper validation |
| Vendor-specific coupling | Medium | Medium | Repository/adapters and domain isolation |
| Backup not validated | Low–Medium | High | Scheduled restore tests |
| Large document growth | Low–Medium | High | No embedded history, bounded guards |
| Cross-tenant query omission | Medium | Critical | Repository signatures and tests |

---

## 19. Rejected Alternatives

### 19.1 PostgreSQL

PostgreSQL is technically an excellent fit.

Advantages include:

- Strong ACID transactions
- Mature operational ecosystem
- Range types
- Exclusion constraints
- Rich SQL
- Referential integrity
- Familiar tooling

It was not selected because:

- Couchbase is an explicit learning objective.
- The document/KV model fits Haven's aggregates.
- The project intentionally explores CAS, N1QL, and document modeling.

PostgreSQL would likely be the strongest alternative if the learning constraint were removed or if complex relational reporting and constraints became dominant.

### 19.2 MongoDB

MongoDB is a valid document database alternative.

It was not selected because:

- Couchbase better aligns with the intended learning goal.
- Couchbase direct KV and N1QL model are attractive for the planned access patterns.
- The project specifically wants Couchbase operational experience.

### 19.3 DynamoDB-Style Storage

Rejected for MVP because:

- Local development is less convenient.
- Query flexibility is lower.
- Access patterns require more up-front denormalization.
- Cloud-managed assumptions would dominate the design.
- The project currently targets Docker-based local infrastructure.

### 19.4 Multiple Databases From Day One

Rejected because polyglot persistence would add unnecessary operational and consistency complexity.

Redis and Kafka are already introduced for distinct, justified roles.

---

## 20. Portability Strategy

The domain and application layers do not depend on Couchbase SDK types.

Portability mechanisms:

- Repository interfaces
- Persistence DTOs
- Explicit mappers
- Neutral version type
- Neutral error categories
- No N1QL in domain/application code
- No Couchbase CAS type in aggregate API
- Migration scripts isolated in infrastructure

Replacing Couchbase would still require significant infrastructure work, but business behavior should remain stable.

---

## 21. Reconsideration Triggers

Revisit this ADR when:

- Relational reporting dominates operational workloads.
- Complex cross-aggregate transactions become common.
- Couchbase transaction latency fails measured targets.
- Required query shapes cannot be indexed efficiently.
- Operational skill or hosting constraints become unacceptable.
- Cost materially exceeds alternatives.
- A managed cloud platform requires another datastore.
- Regulatory requirements require different consistency or backup guarantees.
- The guard-document design becomes operationally fragile.
- PostgreSQL exclusion constraints would substantially simplify correctness.

A new datastore requires a migration ADR.

---

## 22. Migration Strategy

If Haven migrates from Couchbase:

1. Preserve repository contracts.
2. Define target schemas.
3. Build dual-write or change-capture strategy if required.
4. Backfill organizations, resources, and reservations.
5. Rebuild schedule allocation state.
6. Verify reservation and guard consistency.
7. Validate idempotency retention.
8. Rebuild outbox/dedup state as needed.
9. Run shadow reads.
10. Cut traffic gradually.
11. Retain rollback window.
12. Remove Couchbase-specific infrastructure after validation.

Migration must preserve reservation correctness and audit history.

---

## 23. Implementation Impact

### Domain

No Couchbase dependency.

### Application

Depends on repository and transaction-oriented contracts.

### Infrastructure

Must implement:

- Couchbase client lifecycle
- Document mappers
- Repositories
- Transactions
- CAS mapping
- N1QL queries
- Index bootstrap
- Retry classification
- Health checks

### Testing

Requires:

- Couchbase container
- Transaction integration tests
- Query-plan tests
- Concurrency stress tests
- Migration tests
- Backup/restore rehearsal

### Deployment

Requires:

- Couchbase service
- Initialization scripts
- Credentials
- Monitoring
- Backup policy

---

## 24. Validation Criteria

The decision is successful when:

- Direct lookup meets latency targets.
- Search queries use intended indexes.
- Concurrent reservation tests produce one winner.
- No Couchbase type leaks into domain/application code.
- Tenant isolation tests pass.
- Transaction rollback leaves no partial state.
- Outbox and idempotency commit atomically where designed.
- Local bootstrap is repeatable.
- Backup restore is documented and tested.
- Performance remains within initial objectives.

---

## 25. Follow-Up Tasks

- [ ] Create Couchbase bucket/scope/collection bootstrap scripts.
- [ ] Define document mappers.
- [ ] Create schema-version conventions.
- [ ] Implement repository interfaces.
- [ ] Implement direct-key access.
- [ ] Create documented secondary indexes.
- [ ] Implement transaction wrapper.
- [ ] Implement neutral CAS/version mapping.
- [ ] Add overlap query tests.
- [ ] Add schedule guard transaction tests.
- [ ] Add tenant-isolation tests.
- [ ] Add Couchbase readiness health check.
- [ ] Document backup and restore.
- [ ] Benchmark direct KV, N1QL, and transactions.

---

## 26. Interview Notes

### Why Couchbase instead of PostgreSQL?

Couchbase fits the JSON aggregate model, direct KV access, CAS concurrency, N1QL queries, and horizontal evolution. It is also an explicit project learning objective. PostgreSQL remains a strong alternative, especially for exclusion constraints and relational analytics.

### How do you prevent double booking in a document database?

A normal overlap query is insufficient. Haven uses transactional per-resource schedule guard documents together with Reservation, idempotency, and outbox writes.

### Is schema-less storage safe?

Couchbase is flexible-schema, not schema-free. Haven uses document types, schema versions, validation, explicit mappers, migrations, and tests.

### Why not embed reservations in Resource?

Reservation history is unbounded and highly mutable. Embedding would create large hot documents and poor query behavior.

### How do you prevent vendor lock-in?

Repository interfaces, neutral domain types, explicit mapping, and SDK isolation reduce coupling. They do not make migration free, but they protect business logic.

### What is the biggest Couchbase risk?

Incorrect assumptions about query visibility and atomicity. Correctness must rely on direct transactional guard documents, not only N1QL prechecks.

---

## 27. Summary

**Decision:** Use Couchbase as Haven's primary authoritative datastore.

**Reason:** It satisfies the document, key-value, query, concurrency, transaction, and scaling requirements while providing intentional learning value.

**Accepted trade-offs:** More complex interval/concurrency modeling, index discipline, and operational learning than a straightforward relational implementation.

**Strongest alternative:** PostgreSQL.

**Future reconsideration:** Triggered by measured query, transaction, operational, cost, or relational-model limitations.

---

## 28. Completion Checklist

- [x] Context documented
- [x] Problem defined
- [x] Decision drivers ranked
- [x] Alternatives evaluated
- [x] Decision stated
- [x] Rationale documented
- [x] Data model impact included
- [x] Concurrency impact included
- [x] Risks documented
- [x] Portability strategy included
- [x] Migration strategy included
- [x] Reconsideration triggers defined
- [x] Interview notes included

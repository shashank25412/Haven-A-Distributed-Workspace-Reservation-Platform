---
title: Haven Engineering Design Documentation
document: 06 - Database Design
version: 1.0
status: Draft
author: Shashank Kumar
reviewer: Kumar Rahul
last_updated: 2026-07-20
related_documents:
  - 04-domain-model.md
  - 05-api-design.md
  - 08-concurrency.md
  - 09-caching.md
related_adrs:
  - ADR-002-Couchbase
  - ADR-004-OptimisticLocking
---

# Haven — Database Design

## 1. Overview

Couchbase is Haven's authoritative datastore.

This document defines:

- Bucket, scope, and collection strategy
- Document schemas
- Key conventions
- Access patterns
- Index requirements
- Tenant isolation
- CAS usage
- Idempotency persistence
- Event outbox persistence
- Retention and migration strategy

The database model follows query and concurrency requirements rather than relational normalization.

---

## 2. Design Principles

- Keep Resource and Reservation documents separate.
- Every tenant-owned document carries `organizationId`.
- Document keys are opaque and deterministic where useful.
- Unbounded arrays are forbidden.
- Reservation state is authoritative.
- Availability is derived.
- Indexes are designed from access patterns.
- Couchbase SDK types do not leak into domain objects.
- CAS is preserved by infrastructure mappers.
- Schema version is included in persisted documents.

---

## 3. Logical Layout

Recommended local/MVP layout:

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
  integration.outbox
  integration.consumer_offsets_or_dedup
```

A separate bucket per tenant is rejected for MVP because it increases operational overhead.

---

## 4. Document Key Conventions

| Document | Key |
|---|---|
| Organization | `org::<organizationId>` |
| Resource | `resource::<organizationId>::<resourceId>` |
| Reservation | `reservation::<organizationId>::<reservationId>` |
| Idempotency | `idem::<organizationId>::<userId>::<operation>::<keyHash>` |
| Outbox event | `outbox::<organizationId>::<eventId>` |
| Consumer dedup | `consumer::<consumerName>::<eventId>` |

Keys must not contain raw secrets or unbounded user input.

---

## 5. Organization Document

```json
{
  "documentType": "organization",
  "schemaVersion": 1,
  "organizationId": "org_01H...",
  "name": "Example Organization",
  "status": "ACTIVE",
  "timeZone": "Asia/Kolkata",
  "reservationPolicy": {
    "standardMaximumDurationMinutes": 720,
    "maintenanceMaximumDurationMinutes": 1440,
    "allowPastStartToleranceSeconds": 30
  },
  "approvalRules": [
    {
      "resourceType": "MEETING_ROOM",
      "resourceTag": "PRIORITY",
      "approvalRequired": true
    }
  ],
  "createdAt": "2026-07-20T05:30:00Z",
  "updatedAt": "2026-07-20T05:30:00Z"
}
```

Organization policy changes are infrequent and cacheable.

---

## 6. Resource Document

```json
{
  "documentType": "resource",
  "schemaVersion": 1,
  "organizationId": "org_01H...",
  "resourceId": "res_01H...",
  "name": "Orion",
  "description": "Eight-seat meeting room.",
  "resourceType": "MEETING_ROOM",
  "status": "ACTIVE",
  "capacity": 8,
  "location": {
    "building": "Tower A",
    "floor": "5",
    "zone": "East"
  },
  "features": [
    "PROJECTOR",
    "WHITEBOARD"
  ],
  "tags": [
    "PRIORITY"
  ],
  "createdAt": "2026-07-20T05:30:00Z",
  "updatedAt": "2026-07-20T05:30:00Z"
}
```

The document does not embed reservations or store an availability boolean.

---

## 7. Reservation Document

```json
{
  "documentType": "reservation",
  "schemaVersion": 1,
  "organizationId": "org_01H...",
  "reservationId": "rsv_01H...",
  "resourceId": "res_01H...",
  "creatorUserId": "usr_01H...",
  "startTime": "2026-08-01T10:00:00Z",
  "endTime": "2026-08-01T11:00:00Z",
  "status": "CONFIRMED",
  "purpose": "Sprint planning",
  "reservationKind": "STANDARD",
  "approval": {
    "required": false,
    "status": "NOT_REQUIRED",
    "actorUserId": null,
    "decidedAt": null,
    "reason": null
  },
  "createdAt": "2026-07-20T05:30:00Z",
  "updatedAt": "2026-07-20T05:30:00Z"
}
```

Couchbase CAS is captured separately by the SDK result and mapped to an infrastructure `Version`.

---

## 8. Idempotency Document

```json
{
  "documentType": "idempotency",
  "schemaVersion": 1,
  "organizationId": "org_01H...",
  "userId": "usr_01H...",
  "operation": "CREATE_RESERVATION",
  "keyHash": "sha256...",
  "payloadHash": "sha256...",
  "state": "COMPLETED",
  "reservationId": "rsv_01H...",
  "httpStatus": 201,
  "response": {
    "reservationId": "rsv_01H...",
    "status": "CONFIRMED",
    "createdAt": "2026-07-20T05:30:00Z"
  },
  "createdAt": "2026-07-20T05:30:00Z",
  "expiresAt": "2026-07-21T05:30:00Z"
}
```

Possible states:

- `PROCESSING`
- `COMPLETED`
- `FAILED_RETRYABLE`
- `FAILED_FINAL`

The exact atomic relationship with reservation creation is finalized in the concurrency document.

---

## 9. Outbox Document

```json
{
  "documentType": "outbox",
  "schemaVersion": 1,
  "eventId": "evt_01H...",
  "eventType": "ReservationConfirmed",
  "eventVersion": 1,
  "organizationId": "org_01H...",
  "aggregateId": "rsv_01H...",
  "correlationId": "trace_...",
  "occurredAt": "2026-07-20T05:30:00Z",
  "payload": {
    "reservationId": "rsv_01H...",
    "resourceId": "res_01H...",
    "startTime": "2026-08-01T10:00:00Z",
    "endTime": "2026-08-01T11:00:00Z",
    "status": "CONFIRMED"
  },
  "publication": {
    "status": "PENDING",
    "attempts": 0,
    "nextAttemptAt": "2026-07-20T05:30:00Z",
    "publishedAt": null,
    "lastError": null
  }
}
```

Outbox records are retained long enough for recovery and audit, then archived or deleted according to policy.

---

## 10. Access Patterns

| Access Pattern | Collection | Key/Query |
|---|---|---|
| Get organization | organizations | Direct key |
| Get resource | resources | Direct key |
| Search resources | resources | N1QL index |
| Get reservation | reservations | Direct key |
| Find user reservations | reservations | Tenant + user + time/status |
| Find pending approvals | reservations | Tenant + status + time |
| Find blocking overlap for one resource | reservations | Tenant + resource + blocking status + time |
| Find unavailable IDs for candidates | reservations | Tenant + candidate IDs + status + time |
| Resolve idempotency | idempotency | Direct key |
| Poll pending outbox | outbox | Status + next attempt |

---

## 11. Overlap Query

Conceptual predicate:

```sql
SELECT RAW r.resourceId
FROM `haven`.`core`.`reservations` AS r
WHERE r.organizationId = $organizationId
  AND r.resourceId IN $candidateResourceIds
  AND r.status IN ["CONFIRMED"]
  AND r.startTime < $requestedEnd
  AND r.endTime > $requestedStart;
```

For one resource:

```sql
SELECT META(r).id, r.*
FROM `haven`.`core`.`reservations` AS r
WHERE r.organizationId = $organizationId
  AND r.resourceId = $resourceId
  AND r.status IN ["CONFIRMED"]
  AND r.startTime < $requestedEnd
  AND r.endTime > $requestedStart
LIMIT 1;
```

The query detects conflict but by itself does not make the subsequent insert atomic.

---

## 12. Indexes

Index names are illustrative.

### 12.1 Resource Search Index

```sql
CREATE INDEX idx_resources_search
ON `haven`.`core`.`resources`
(
  organizationId,
  resourceType,
  status,
  capacity,
  location.building,
  location.floor
)
WHERE documentType = "resource";
```

Feature arrays may require an array index:

```sql
CREATE INDEX idx_resources_features
ON `haven`.`core`.`resources`
(
  organizationId,
  resourceType,
  DISTINCT ARRAY f FOR f IN features END
)
WHERE documentType = "resource"
  AND status = "ACTIVE";
```

### 12.2 Reservation Overlap Index

```sql
CREATE INDEX idx_reservations_overlap
ON `haven`.`core`.`reservations`
(
  organizationId,
  resourceId,
  status,
  startTime,
  endTime
)
WHERE documentType = "reservation";
```

### 12.3 User Reservation Index

```sql
CREATE INDEX idx_reservations_user
ON `haven`.`core`.`reservations`
(
  organizationId,
  creatorUserId,
  startTime,
  status
)
WHERE documentType = "reservation";
```

### 12.4 Pending Approval Index

```sql
CREATE INDEX idx_reservations_pending
ON `haven`.`core`.`reservations`
(
  organizationId,
  status,
  createdAt
)
WHERE documentType = "reservation"
  AND status = "PENDING_APPROVAL";
```

### 12.5 Outbox Polling Index

```sql
CREATE INDEX idx_outbox_pending
ON `haven`.`integration`.`outbox`
(
  publication.status,
  publication.nextAttemptAt,
  occurredAt
)
WHERE documentType = "outbox";
```

Index effectiveness must be verified with `EXPLAIN` and representative data.

---

## 13. Atomic Reservation Strategy

A read-overlap-then-insert sequence can race.

The database layer must implement an atomic protection mechanism.

Candidate strategies:

1. Time-bucket guard documents per resource
2. Couchbase transaction covering guard, reservation, idempotency, and outbox
3. Serializable reservation ledger abstraction
4. External lock

Recommended MVP direction:

- Use Couchbase transactions or carefully designed guard documents.
- Use CAS for updates to existing reservations.
- Avoid treating a normal N1QL precheck as sufficient.

The final algorithm is specified in `08-concurrency.md`.

---

## 14. Tenant Isolation

Every N1QL query must include `organizationId`.

Repository methods require organization context.

Document keys also include organization ID to:

- Prevent accidental collisions
- Make direct-key access tenant-specific
- Improve operational diagnosis

Application authorization remains mandatory even with tenant-scoped keys.

---

## 15. Serialization and Mapping

Infrastructure mappers translate:

```text
Couchbase JSON document
<-> Persistence DTO
<-> Domain aggregate
```

Rules:

- Missing required fields fail safely.
- Unknown additive fields are tolerated when possible.
- Schema version selects migration/rehydration path.
- Domain creation factories are not reused for persistence rehydration.
- CAS is stored outside domain JSON.

---

## 16. Schema Evolution

Every document includes `schemaVersion`.

Evolution strategies:

- Add optional fields with defaults.
- Use read-time migration for small changes.
- Use background migration for index-sensitive or large changes.
- Never repurpose existing fields.
- Preserve backward compatibility during rolling deployment.

Migration scripts live under:

```text
src/infrastructure/couchbase/migrations/
```

---

## 17. Retention

### Reservations

Retained for audit and reporting according to future policy.

### Idempotency

Initial recommendation: 24 hours for create requests, configurable.

### Outbox

Retain published records for operational recovery, then archive/delete.

### Consumer Deduplication

Retain at least as long as the event replay window.

---

## 18. Consistency Levels

Authoritative write/read-after-write paths may require stronger query consistency than general search.

Use KV direct access where possible.

N1QL scan consistency should be selected deliberately:

- Search can tolerate bounded staleness.
- Immediate user confirmation retrieval should prefer direct key.
- Conflict protection must not rely on eventually indexed query visibility alone.

This distinction is critical.

---

## 19. Backup and Recovery

Future production requirements:

- Couchbase backup schedule
- Restore validation
- Index recreation scripts
- Outbox replay
- Idempotency retention awareness
- Recovery runbook

MVP local development uses seeded/bootstrap scripts.

---

## 20. Performance Risks

- Broad `IN` candidate lists
- Large offset pagination
- Array feature indexes
- High-cardinality tenant/resource queries
- N1QL index lag
- Hot guard documents
- Large stored idempotency responses
- Outbox polling scans

Each risk requires benchmark and query-plan validation.

---

## 21. Alternatives

### Relational PostgreSQL

Strong fit for exclusion constraints and transactions, but Couchbase is selected as a project learning goal and document database design exercise.

### One Collection for All Documents

Rejected because collection-specific indexes and operational ownership are clearer with separated collections.

### Embed Reservation History

Rejected due to unbounded growth and contention.

### Store Availability Documents

Deferred until measured read scale justifies a projection.

---

## 22. Test Requirements

- Direct-key round trip
- Serialization compatibility
- Tenant isolation
- Overlap query matrix
- Index creation
- Query plan assertion where practical
- CAS conflict
- Transaction rollback
- Idempotency atomicity
- Outbox persistence
- Schema-version migration

---

## 23. Interview Discussion Notes

### Why include organization ID in both key and body?

The key prevents collisions and enables scoped direct lookup; the body supports queries, auditing, and validation. Infrastructure verifies consistency.

### Why is N1QL precheck insufficient?

Two requests can both observe no conflict before either inserts. Atomic write protection must serialize conflicting allocation at a narrower boundary.

### Why use direct KV operations where possible?

They provide predictable latency and avoid index visibility concerns for known IDs.

### Why separate outbox collection?

It has a different lifecycle, polling pattern, and retention policy from domain aggregates.

---

## 24. Summary

Couchbase stores separate organization, resource, reservation, idempotency, and outbox documents.

Indexes support tenant-scoped search and overlap queries, while atomic reservation correctness is delegated to a concurrency-safe transaction or guard design.

---

## 25. Next Document

The next document is:

```text
docs/07-event-driven-design.md
```

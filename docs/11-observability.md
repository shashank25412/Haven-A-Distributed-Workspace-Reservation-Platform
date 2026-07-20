---
title: Haven Engineering Design Documentation
document: 11 - Observability
version: 1.0
status: Draft
author: Shashank Kumar
reviewer: Kumar Rahul
last_updated: 2026-07-20
related_documents:
  - 02-high-level-design.md
  - 07-event-driven-design.md
  - 10-security.md
  - 13-performance.md
---

# Haven — Observability Design

## 1. Overview

Haven must be diagnosable without attaching a debugger to production.

The observability model combines:

- Structured logs
- Metrics
- Distributed traces
- Health checks
- Dashboards
- Alerts
- Business outcome telemetry

---

## 2. Goals

- Trace one request across layers and asynchronous events.
- Distinguish expected conflicts from technical failures.
- Detect dependency degradation.
- Measure contention and idempotency.
- Diagnose consumer lag and event loss risk.
- Avoid sensitive-data leakage.
- Support local development and future production operation.

---

## 3. Correlation Model

Identifiers:

- `requestId`: one inbound HTTP request
- `traceId`: distributed trace
- `spanId`: one operation
- `correlationId`: business flow across async boundaries
- `causationId`: command/event that caused another event
- `reservationId`
- `resourceId`
- `organizationId`

Kafka events carry trace/correlation context.

---

## 4. Structured Logging

Recommended JSON fields:

```json
{
  "timestamp": "2026-07-20T05:30:00Z",
  "level": "INFO",
  "service": "haven",
  "environment": "local",
  "operation": "CreateReservation",
  "message": "Reservation created",
  "requestId": "req_...",
  "traceId": "trace_...",
  "organizationId": "org_...",
  "userId": "usr_...",
  "reservationId": "rsv_...",
  "resourceId": "res_...",
  "status": "CONFIRMED",
  "durationMs": 42
}
```

Do not log purpose by default.

---

## 5. Log Levels

| Level | Use |
|---|---|
| TRACE | Fine-grained development tracing |
| DEBUG | Diagnostic state safe for non-production or sampled use |
| INFO | Meaningful lifecycle and operational events |
| WARN | Recoverable degradation or retry |
| ERROR | Failed operation requiring investigation |
| CRITICAL | Process cannot continue safely |

A reservation conflict is normally INFO or a dedicated business metric, not ERROR.

---

## 6. Logging Boundaries

### Presentation

- Request accepted/completed
- Route
- Status
- Latency
- Request size
- Authentication outcome

### Application

- Use-case result
- Retry count
- Idempotency outcome
- Authorization denial
- Business conflict

### Infrastructure

- Couchbase operation timing
- Redis hit/miss/failure
- Kafka publication
- Consumer processing
- Circuit breaker state

### Domain

No direct spdlog dependency.

---

## 7. Metrics

### HTTP

- `http_requests_total`
- `http_request_duration_seconds`
- `http_in_flight_requests`
- `http_request_size_bytes`
- `http_response_size_bytes`

Labels remain low-cardinality:

- Method
- Route template
- Status class
- Outcome category

### Reservation

- `reservation_create_total`
- `reservation_conflict_total`
- `reservation_pending_approval_total`
- `reservation_cancel_total`
- `reservation_extension_total`
- `reservation_approval_total`
- `reservation_rejection_total`

### Concurrency

- `reservation_transaction_retry_total`
- `reservation_transaction_abort_total`
- `reservation_cas_conflict_total`
- `reservation_retry_exhausted_total`

### Idempotency

- `idempotency_hit_total`
- `idempotency_mismatch_total`
- `idempotency_in_progress_total`

### Dependencies

- Couchbase latency/errors
- Redis hit/miss/bypass/errors
- Kafka publish latency/errors
- Consumer lag/retries/DLQ

---

## 8. Metric Cardinality

Do not use as metric labels:

- Reservation ID
- Resource ID
- User ID
- Trace ID
- Idempotency key

Organization ID may also be too high-cardinality at future scale.

Use logs and traces for individual identifiers.

---

## 9. Tracing

Recommended spans:

```text
HTTP POST /reservations
  auth.validate
  idempotency.lookup
  resource.load
  organization.loadPolicy
  reservation.transaction
    guard.load
    reservation.insert
    outbox.insert
    idempotency.complete
```

Async trace:

```text
outbox.publish
kafka.consume
notification.send
```

Trace sampling should preserve errors and selected slow requests.

---

## 10. Health Checks

### Liveness

Checks process health only.

### Readiness

Checks essential ability to serve traffic.

Suggested readiness:

- Couchbase required
- Redis optional/degraded
- Kafka readiness depends on outbox capacity and policy
- Configuration valid

### Startup

Optional startup probe indicates initialization and index/bootstrap completion.

---

## 11. Dashboards

### API Dashboard

- Request rate
- p50/p95/p99 latency
- Error rate
- Rate limiting
- In-flight requests

### Reservation Dashboard

- Confirmed vs pending
- Conflict rate
- Approval rate
- Cancellation rate
- Transaction retries

### Dependency Dashboard

- Couchbase latency/errors
- Redis hit ratio
- Kafka publish health
- Consumer lag
- Outbox backlog

---

## 12. Alerts

Candidate alerts:

- Readiness unavailable
- Elevated `5xx`
- Create latency above threshold
- Couchbase timeout spike
- Outbox oldest pending age high
- DLQ growth
- Consumer lag high
- Transaction retry rate abnormal
- Redis circuit open for extended period

Alerts should be actionable and avoid noise.

---

## 13. SLO Foundations

Future SLO examples:

- API availability
- Create reservation successful technical processing
- Search latency
- Event publication delay
- Notification delivery delay

Business conflict responses are excluded from technical availability error budgets.

---

## 14. Sensitive Data

Redact:

- JWT
- Authorization header
- Secrets
- Email/phone unless explicitly approved
- Purpose
- Raw request bodies
- Raw dependency stack traces in public responses

Logging configuration itself must be reviewed.

---

## 15. Local Development

Local environment should provide:

- Console structured logs
- Configurable log level
- Optional Prometheus endpoint
- Optional Jaeger/OpenTelemetry collector
- Kafka UI
- Health endpoint

Observability tooling should be optional for minimal startup but documented.

---

## 16. Failure Diagnosis Examples

### Conflict Spike

Check:

- Resource hot spots in sampled logs
- Recent policy/resource changes
- Search freshness
- Transaction retry rate

### Outbox Backlog

Check:

- Kafka health
- Relay errors
- Oldest pending event
- Producer configuration
- Poison event

### Search Latency

Check:

- Couchbase query plan
- Redis hit ratio
- Candidate set size
- Pagination
- N1QL index utilization

---

## 17. Tests

- Log redaction
- Trace propagation
- Metric increment
- Health dependency behavior
- Outbox backlog metric
- Redis degraded readiness
- Error classification
- Route-template labels instead of raw path

---

## 18. Alternatives

### Logs Only

Rejected because logs do not efficiently reveal trends or latency distributions.

### Metrics with High-Cardinality IDs

Rejected due to cost and instability.

### Domain Logging Directly

Rejected to preserve domain independence.

---

## 19. Interview Discussion Notes

### How do you distinguish a business failure from system failure?

Use separate outcome categories and metrics. A 409 overlap is expected contention; a 503 Couchbase failure is technical failure.

### Why carry correlation IDs into Kafka?

Asynchronous processing occurs after the HTTP request. Correlation allows end-to-end diagnosis.

### Why not label metrics by tenant?

It may create high cardinality and privacy concerns. Use controlled aggregation or logs.

---

## 20. Summary

Haven uses structured, correlated telemetry across HTTP, application, persistence, cache, outbox, Kafka, and consumers.

Observability is designed as part of the system rather than added after failure.

---

## 21. Next Document

```text
docs/12-testing.md
```

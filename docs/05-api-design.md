---
title: Haven Engineering Design Documentation
document: 05 - API Design
version: 1.0
status: Draft
author: Shashank Kumar
reviewer: Kumar Rahul
last_updated: 2026-07-20
related_documents:
  - 01-requirements.md
  - 02-high-level-design.md
  - 03-low-level-design.md
  - 04-domain-model.md
  - 10-security.md
---

# Haven — API Design

## 1. Overview

This document defines the Haven MVP REST API contract.

Base path:

```text
/api/v1
```

Principles:

- Resource-oriented URLs
- Task-oriented action endpoints for lifecycle transitions
- JWT authentication
- Tenant identity derived from trusted caller context
- Server-generated reservation IDs
- Client-supplied idempotency keys
- UTC ISO-8601 timestamps
- Consistent error contracts
- Bounded pagination
- OpenAPI as the machine-readable source of the public contract

---

## 2. API Conventions

### 2.1 Authentication

Protected APIs require:

```http
Authorization: Bearer <JWT>
```

### 2.2 Content Type

```http
Content-Type: application/json
Accept: application/json
```

Errors may use:

```http
Content-Type: application/problem+json
```

### 2.3 Time

All timestamps use UTC ISO-8601:

```text
2026-08-01T10:00:00Z
```

### 2.4 Identifiers

Identifiers are opaque strings.

Clients must not derive meaning from ID format.

### 2.5 Naming

JSON fields use `camelCase`.

Enums use stable uppercase snake case.

### 2.6 Correlation

Responses should return:

```http
X-Request-Id: <id>
Traceparent: <trace-context>
```

where supported.

---

## 3. Common Error Contract

```json
{
  "code": "RESERVATION_CONFLICT",
  "message": "The resource is no longer available for the requested interval.",
  "details": [
    {
      "field": "timeInterval",
      "reason": "OVERLAPPING_RESERVATION"
    }
  ],
  "traceId": "4d7a1ed4cdbb4b5e"
}
```

Fields:

| Field | Required | Meaning |
|---|---:|---|
| `code` | Yes | Stable machine-readable error |
| `message` | Yes | Safe human-readable summary |
| `details` | No | Field or policy details |
| `traceId` | Yes | Operational correlation |

---

## 4. Pagination

MVP uses bounded page pagination:

```text
page=1
pageSize=20
```

Limits:

- Default page size: 20
- Maximum page size: 100
- Page must be positive

Response shape:

```json
{
  "items": [],
  "pagination": {
    "page": 1,
    "pageSize": 20,
    "totalItems": 42,
    "totalPages": 3
  }
}
```

Cursor pagination may replace this for high-scale search after measurement.

---

## 5. Resource APIs

### 5.1 Search Available Resources

```http
GET /api/v1/resources
```

Required query parameters:

| Parameter | Type | Description |
|---|---|---|
| `resourceType` | enum | Required resource category |
| `startTime` | timestamp | Requested start |
| `endTime` | timestamp | Requested end |

Optional:

| Parameter | Type |
|---|---|
| `minimumCapacity` | integer |
| `building` | string |
| `floor` | string |
| `features` | comma-separated strings |
| `sort` | enum |
| `page` | integer |
| `pageSize` | integer |

Example:

```http
GET /api/v1/resources?resourceType=MEETING_ROOM&startTime=2026-08-01T10%3A00%3A00Z&endTime=2026-08-01T11%3A00%3A00Z&minimumCapacity=8&features=PROJECTOR,WHITEBOARD&page=1&pageSize=20
```

Success:

```http
200 OK
```

```json
{
  "items": [
    {
      "resourceId": "res_01H...",
      "name": "Orion",
      "description": "Eight-seat meeting room near reception.",
      "resourceType": "MEETING_ROOM",
      "capacity": 8,
      "location": {
        "building": "Tower A",
        "floor": "5",
        "zone": "East"
      },
      "features": [
        "PROJECTOR",
        "WHITEBOARD"
      ]
    }
  ],
  "searchContext": {
    "startTime": "2026-08-01T10:00:00Z",
    "endTime": "2026-08-01T11:00:00Z"
  },
  "pagination": {
    "page": 1,
    "pageSize": 20,
    "totalItems": 1,
    "totalPages": 1
  }
}
```

Meaningful errors:

- `400` invalid query
- `401` invalid authentication
- `422` invalid interval or unsupported resource type
- `429` rate limit
- `503` authoritative search dependency unavailable

Every returned resource is available according to the search snapshot. Final booking is not guaranteed.

### 5.2 Get Resource Details

```http
GET /api/v1/resources/{resourceId}
```

Success:

```json
{
  "resourceId": "res_01H...",
  "name": "Orion",
  "description": "Eight-seat meeting room near reception.",
  "resourceType": "MEETING_ROOM",
  "capacity": 8,
  "status": "ACTIVE",
  "location": {
    "building": "Tower A",
    "floor": "5",
    "zone": "East"
  },
  "features": [
    "PROJECTOR",
    "WHITEBOARD"
  ]
}
```

Errors:

- `401`
- `404`
- `503`

The response does not assert availability for an unspecified interval.

---

## 6. Reservation APIs

### 6.1 Create Reservation

```http
POST /api/v1/reservations
```

Headers:

```http
Authorization: Bearer <JWT>
Idempotency-Key: <UUID-or-opaque-key>
```

Request:

```json
{
  "resourceId": "res_01H...",
  "startTime": "2026-08-01T10:00:00Z",
  "endTime": "2026-08-01T11:00:00Z",
  "purpose": "Sprint planning"
}
```

The server derives:

- Organization ID
- Creator user ID
- Roles and authorization

Success:

```http
201 Created
Location: /api/v1/reservations/rsv_01H...
```

Auto-confirmed response:

```json
{
  "reservationId": "rsv_01H...",
  "resourceId": "res_01H...",
  "status": "CONFIRMED",
  "startTime": "2026-08-01T10:00:00Z",
  "endTime": "2026-08-01T11:00:00Z",
  "createdAt": "2026-07-20T05:30:00Z"
}
```

Approval response:

```json
{
  "reservationId": "rsv_01H...",
  "resourceId": "res_01H...",
  "status": "PENDING_APPROVAL",
  "startTime": "2026-08-01T10:00:00Z",
  "endTime": "2026-08-01T11:00:00Z",
  "createdAt": "2026-07-20T05:30:00Z"
}
```

Errors:

| Code | HTTP |
|---|---:|
| `VALIDATION_ERROR` | 400 |
| `UNAUTHENTICATED` | 401 |
| `RESOURCE_NOT_FOUND` | 404 |
| `RESERVATION_CONFLICT` | 409 |
| `IDEMPOTENCY_KEY_MISMATCH` | 409 |
| `RESERVATION_DURATION_EXCEEDED` | 422 |
| `RESOURCE_INACTIVE` | 422 |
| `RATE_LIMITED` | 429 |
| `DEPENDENCY_UNAVAILABLE` | 503 |

### 6.2 Get Reservation

```http
GET /api/v1/reservations/{reservationId}
```

Response:

```json
{
  "reservationId": "rsv_01H...",
  "resource": {
    "resourceId": "res_01H...",
    "name": "Orion",
    "resourceType": "MEETING_ROOM"
  },
  "creator": {
    "userId": "usr_01H...",
    "displayName": "Shashank Kumar"
  },
  "startTime": "2026-08-01T10:00:00Z",
  "endTime": "2026-08-01T11:00:00Z",
  "purpose": "Sprint planning",
  "status": "CONFIRMED",
  "approval": {
    "required": false,
    "status": "NOT_REQUIRED"
  },
  "createdAt": "2026-07-20T05:30:00Z",
  "updatedAt": "2026-07-20T05:30:00Z"
}
```

Errors:

- `401`
- `404`
- `503`

### 6.3 List My Reservations

```http
GET /api/v1/reservations/me
```

Optional query:

- `status`
- `from`
- `to`
- `upcoming`
- `page`
- `pageSize`
- `sort`

Response uses the common paginated shape.

### 6.4 Cancel Reservation

```http
POST /api/v1/reservations/{reservationId}/cancel
```

Optional request:

```json
{
  "reason": "Meeting cancelled"
}
```

Success:

```http
200 OK
```

```json
{
  "reservationId": "rsv_01H...",
  "status": "CANCELLED",
  "cancelledAt": "2026-07-25T08:00:00Z"
}
```

Cancellation is idempotent from the caller's perspective when the same final state already exists.

Errors:

- `401`
- `403`
- `404`
- `409 INVALID_RESERVATION_STATE`
- `503`

### 6.5 Extend Reservation

```http
POST /api/v1/reservations/{reservationId}/extend
```

Request:

```json
{
  "newEndTime": "2026-08-01T12:00:00Z"
}
```

Success:

```json
{
  "reservationId": "rsv_01H...",
  "status": "CONFIRMED",
  "startTime": "2026-08-01T10:00:00Z",
  "endTime": "2026-08-01T12:00:00Z",
  "updatedAt": "2026-07-25T08:00:00Z"
}
```

Errors:

- `403`
- `404`
- `409 RESERVATION_CONFLICT`
- `409 CONCURRENT_MODIFICATION`
- `422 INVALID_EXTENSION`
- `422 RESERVATION_DURATION_EXCEEDED`
- `503`

---

## 7. Approval APIs

### 7.1 List Pending Approvals

```http
GET /api/v1/approvals
```

Optional:

- `resourceType`
- `from`
- `to`
- `page`
- `pageSize`
- `sort`

Response:

```json
{
  "items": [
    {
      "reservationId": "rsv_01H...",
      "resource": {
        "resourceId": "res_01H...",
        "name": "Executive Boardroom",
        "resourceType": "MEETING_ROOM"
      },
      "creator": {
        "userId": "usr_01H...",
        "displayName": "User Name"
      },
      "startTime": "2026-08-01T10:00:00Z",
      "endTime": "2026-08-01T11:00:00Z",
      "purpose": "Quarterly review",
      "status": "PENDING_APPROVAL",
      "createdAt": "2026-07-20T05:30:00Z"
    }
  ],
  "pagination": {
    "page": 1,
    "pageSize": 20,
    "totalItems": 1,
    "totalPages": 1
  }
}
```

### 7.2 Approve Reservation

```http
POST /api/v1/approvals/{reservationId}/approve
```

Optional request:

```json
{
  "comment": "Approved for the leadership review."
}
```

Success:

```json
{
  "reservationId": "rsv_01H...",
  "status": "CONFIRMED",
  "approvedAt": "2026-07-21T09:00:00Z"
}
```

Approval performs final conflict detection.

Errors:

- `403`
- `404`
- `409 RESERVATION_CONFLICT`
- `409 INVALID_RESERVATION_STATE`
- `409 CONCURRENT_MODIFICATION`
- `503`

### 7.3 Reject Reservation

```http
POST /api/v1/approvals/{reservationId}/reject
```

Request:

```json
{
  "reason": "Reserved for an executive event."
}
```

Success:

```json
{
  "reservationId": "rsv_01H...",
  "status": "REJECTED",
  "rejectedAt": "2026-07-21T09:00:00Z",
  "reason": "Reserved for an executive event."
}
```

---

## 8. Organization API

### Get Effective Policies

```http
GET /api/v1/organization/policies
```

Response:

```json
{
  "standardMaximumDurationHours": 12,
  "maintenanceMaximumDurationHours": 24,
  "approvalRules": [
    {
      "resourceType": "MEETING_ROOM",
      "resourceTag": "PRIORITY",
      "approvalRequired": true
    }
  ],
  "timeZone": "Asia/Kolkata"
}
```

This endpoint is informational. Server-side policy remains authoritative.

---

## 9. Health APIs

### 9.1 Liveness

```http
GET /health/live
```

```json
{
  "status": "UP"
}
```

### 9.2 Readiness

```http
GET /health/ready
```

Ready:

```json
{
  "status": "UP",
  "dependencies": {
    "couchbase": "UP",
    "redis": "DEGRADED",
    "kafka": "UP"
  }
}
```

Redis may be degraded without making the service unready if safe fallback exists.

---

## 10. Validation Rules

### Create Reservation

- `resourceId` required
- `startTime` required
- `endTime` required
- Valid UTC timestamps
- `startTime < endTime`
- Purpose bounded in length
- Idempotency key required and bounded
- Unknown JSON fields handled according to compatibility policy

### Search

- `resourceType` required
- Valid interval
- Page bounds
- Feature count bounded
- Minimum capacity non-negative

### Reject

- Reason required or strongly recommended according to final policy
- Reason length bounded

---

## 11. Idempotency Semantics

Scope:

```text
organization + authenticated user + operation + idempotency key
```

Rules:

- Same key and same canonical payload: return original result.
- Same key and different payload: `409`.
- Concurrent same-key requests: one processes; others wait boundedly or receive stable in-progress response.
- Server stores enough response data to reconstruct the original external result.
- Key retention is defined in the concurrency design.

---

## 12. Compatibility and Versioning

- Breaking changes require a new API version.
- Additive optional response fields are allowed.
- Enum additions require tolerant client guidance.
- Fields are not repurposed.
- Deprecated fields receive a documented removal path.
- OpenAPI changes are reviewed in CI.

---

## 13. Security Rules

- Organization and creator are not accepted from request body.
- Cross-tenant access does not reveal object existence.
- JWT and secrets never appear in logs.
- Approval endpoints require explicit permission.
- Rate limits protect expensive search and write endpoints.
- Error messages remain safe.

---

## 14. API Alternatives

### PUT `/create-reservation`

Rejected because the client does not own the reservation URI or ID.

### DELETE Reservation

Rejected because cancellation is a state transition and history is preserved.

### GET With Request Body

Rejected because intermediary support and tooling are inconsistent.

### Generic PATCH Reservation

Deferred because named actions provide clearer state-transition semantics for MVP.

---

## 15. OpenAPI Skeleton

```yaml
openapi: 3.1.0
info:
  title: Haven API
  version: 1.0.0
servers:
  - url: /api/v1
security:
  - bearerAuth: []
paths:
  /resources:
    get:
      summary: Search available resources
  /reservations:
    post:
      summary: Create a reservation
      parameters:
        - in: header
          name: Idempotency-Key
          required: true
          schema:
            type: string
  /reservations/{reservationId}:
    get:
      summary: Get a reservation
  /reservations/{reservationId}/cancel:
    post:
      summary: Cancel a reservation
  /reservations/{reservationId}/extend:
    post:
      summary: Extend a reservation
  /approvals:
    get:
      summary: List pending approvals
  /approvals/{reservationId}/approve:
    post:
      summary: Approve a reservation
  /approvals/{reservationId}/reject:
    post:
      summary: Reject a reservation
components:
  securitySchemes:
    bearerAuth:
      type: http
      scheme: bearer
      bearerFormat: JWT
```

A complete generated OpenAPI file should be stored under `presentation/openapi/`.

---

## 16. Contract Test Requirements

Every endpoint requires tests for:

- Authentication
- Tenant isolation
- Required fields
- Invalid timestamp
- Expected success status
- Error body shape
- Unknown identifier
- Dependency unavailable
- Trace ID presence

Create reservation additionally tests:

- Idempotent replay
- Payload mismatch
- Conflict
- Pending approval
- Auto-confirmation

---

## 17. Interview Discussion Notes

### Why action endpoints?

Cancellation, extension, approval, and rejection express domain transitions that are clearer than generic update payloads.

### Why is resource type mandatory for search?

It bounds the query, clarifies metadata semantics, and matches the client journey across heterogeneous resource categories.

### Why `201` for pending approval?

The reservation resource exists immediately, even though confirmation is pending.

### Why not include `available: true`?

The endpoint already filters by the requested interval. The returned list implicitly represents the search snapshot.

---

## 18. Summary

The Haven API is small, explicit, tenant-aware, idempotent where required, and aligned with the reservation lifecycle.

Search discovers resources; create reservation makes the final authoritative allocation decision.

---

## 19. Next Document

The next document is:

```text
docs/06-database-design.md
```

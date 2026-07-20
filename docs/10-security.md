---
title: Haven Engineering Design Documentation
document: 10 - Security
version: 1.0
status: Draft
author: Shashank Kumar
reviewer: Kumar Rahul
last_updated: 2026-07-20
related_documents:
  - 01-requirements.md
  - 02-high-level-design.md
  - 05-api-design.md
  - 06-database-design.md
related_adrs:
  - ADR-007-JWT-Authentication
---

# Haven — Security Design

## 1. Overview

This document defines Haven's MVP security model.

Security objectives:

- Authenticate callers.
- Authorize business actions.
- Enforce tenant isolation.
- Protect APIs and dependencies.
- Prevent sensitive data leakage.
- Provide auditability.
- Fail safely.

---

## 2. Threat Model

Relevant threats include:

- Forged or expired JWT
- Cross-tenant identifier access
- Privilege escalation
- Reservation creation on behalf of another user
- Injection through filters or JSON
- Enumeration of resource/reservation IDs
- Replay of create requests
- Rate abuse
- Sensitive log leakage
- Stolen infrastructure credentials
- Malicious or malformed Kafka events
- Dependency access from untrusted networks

---

## 3. Authentication

Protected requests require a signed JWT.

Validation includes:

- Signature
- Expiration
- Not-before where present
- Issuer
- Audience
- Supported algorithm
- Key rotation metadata

Caller context:

```text
userId
organizationId
roles
permissions
tokenId where useful
```

The raw JWT is never passed into the domain.

---

## 4. Authorization

Authorization occurs in the application boundary.

Examples:

- Standard user can create for self.
- Creator/admin can cancel according to policy.
- Approver permission required for approval.
- Tenant membership required for every resource/reservation access.
- Maintenance duration requires explicit permission.

Authorization is not inferred from free-form purpose.

---

## 5. Tenant Isolation

Defense in depth:

1. Organization ID derived from trusted token context.
2. Repository methods require organization ID.
3. Couchbase queries include organization predicate.
4. Document keys contain organization ID.
5. Redis keys contain organization ID.
6. Events carry organization ID.
7. Tests attempt cross-tenant access.

For hidden cross-tenant objects, return safe `404` behavior where appropriate.

---

## 6. Request Validation

Validate:

- Content length
- JSON structure
- Identifier format
- Timestamp format
- Enum values
- Text bounds
- Pagination bounds
- Feature count
- Header length
- Idempotency key length and character policy

N1QL values use parameterized queries.

No string concatenation with untrusted filters.

---

## 7. Idempotency and Replay Protection

Idempotency prevents accidental duplicate create requests.

It is not a substitute for authentication.

The key is scoped by user, tenant, and operation.

Retention is bounded.

Mismatch returns a stable conflict without revealing internal hashes.

---

## 8. API Security Headers

Production gateway or server should provide:

- `Strict-Transport-Security`
- `X-Content-Type-Options: nosniff`
- Appropriate `Content-Security-Policy` for served docs/UI
- `Cache-Control: no-store` on sensitive responses where applicable
- Controlled CORS policy

TLS is mandatory in production.

---

## 9. Rate Limiting

Protect:

- Login/token endpoints
- Resource search
- Reservation create
- Approval actions
- Health detail endpoints

Rate limits use combinations of:

- IP
- User
- Tenant
- Endpoint

Responses use `429` and may include `Retry-After`.

---

## 10. Secrets Management

Secrets include:

- Couchbase credentials
- Redis credentials
- Kafka credentials
- JWT verification configuration
- TLS keys

Rules:

- Never commit secrets.
- Use environment variables locally.
- Use secret manager in production.
- Avoid printing configuration values.
- Rotate credentials.
- Apply least privilege.

---

## 11. Dependency Security

### Couchbase

- Private network
- Scoped credentials
- TLS in production
- Least-privilege collection access
- Backup access restricted

### Redis

- Private network
- Authentication
- TLS where supported
- Disable unsafe commands where practical
- No public exposure

### Kafka

- Authenticated producer/consumer
- Topic ACLs
- TLS
- Separate credentials by role
- DLQ access restricted

---

## 12. Event Security

Events must not include:

- JWT
- Password
- Phone number unless required and approved
- Email unless consumer contract requires it
- Infrastructure credentials
- Internal exception stack traces

Consumers validate:

- Event type
- Version
- Required fields
- Tenant identity
- Payload bounds

---

## 13. Logging and Privacy

Never log:

- Authorization header
- Full JWT
- Secrets
- Passwords
- Full sensitive request bodies
- Raw provider errors containing credentials

Use stable IDs for diagnosis.

Purpose may contain user-entered text and should not be logged by default.

---

## 14. Audit Events

Security-relevant actions record:

- Actor user ID
- Organization ID
- Operation
- Target ID
- Outcome
- Timestamp
- Trace ID
- Reason where safe

Examples:

- Approval
- Rejection
- Cancellation
- Authorization denial
- Policy change
- Repeated idempotency mismatch

---

## 15. Error Handling

Public errors:

- Do not expose stack traces.
- Do not expose database names or query text.
- Do not confirm cross-tenant existence.
- Use stable error codes.
- Include trace ID.
- Log richer safe internal context separately.

---

## 16. OpenAPI and Swagger Security

- Swagger UI may be disabled or protected in production.
- Example tokens are never committed.
- OpenAPI clearly marks protected operations.
- Sensitive internal health detail is not publicly exposed.

---

## 17. Supply Chain Security

- Pin vcpkg dependencies.
- Review dependency licenses.
- Run vulnerability scanning.
- Use trusted container base images.
- Generate SBOM when productionized.
- Keep compiler and dependencies patched.
- Restrict GitHub Actions permissions.

---

## 18. C++ Security Practices

- RAII
- Bounds-aware APIs
- No raw owning pointers
- Avoid unsafe C APIs
- Validate sizes before allocation
- Avoid integer narrowing
- Use sanitizers in CI/debug
- Treat external data as untrusted
- Avoid format-string misuse

---

## 19. Security Tests

- Invalid/expired token
- Wrong issuer/audience
- Missing permission
- Cross-tenant resource
- Cross-tenant reservation
- Query injection attempt
- Oversized payload
- Malformed enum
- Rate limit
- Idempotency replay/mismatch
- Log redaction
- Event schema tampering

---

## 20. Incident Readiness

Future production runbooks should cover:

- Credential compromise
- JWT signing-key rotation
- Cross-tenant exposure investigation
- Malicious consumer event
- Dependency compromise
- Secret leakage
- Emergency token revocation

---

## 21. Alternatives

### Organization ID in Request Body

Rejected because trusted token context already owns tenant identity.

### Role Checks Only in Controllers

Rejected because alternate presentation adapters could bypass them.

### Public Detailed Health Endpoint

Rejected because it exposes infrastructure information.

---

## 22. Interview Discussion Notes

### How is multi-tenancy protected?

Through trusted tenant context, authorization, scoped repositories, keys, queries, caches, events, and tests.

### Why return 404 for some forbidden objects?

It avoids disclosing whether another tenant's object exists.

### Why keep authorization in application rather than domain?

Authorization often requires caller context and use-case policy. Domain still protects intrinsic invariants.

---

## 23. Summary

Haven uses JWT authentication, application-level authorization, tenant-scoped persistence, strict validation, rate limiting, secret isolation, safe errors, and auditability.

---

## 24. Next Document

```text
docs/11-observability.md
```

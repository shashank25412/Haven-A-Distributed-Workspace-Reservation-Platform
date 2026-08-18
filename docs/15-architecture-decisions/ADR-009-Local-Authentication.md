---
title: ADR-009 - Transitional Local Authentication
status: Accepted
date: 2026-08-18
supersedes: []
superseded_by: null
---

# ADR-009: Transitional Local Authentication

## Context

ADR-007 defines external OpenID Connect as Haven's production target. The
current self-hosted application also needs registration and authentication
before that provider integration is available.

## Decision

Haven provides a bounded local identity service with opaque bearer sessions.
Identity data is isolated in the `haven_identity` Couchbase bucket, under the
`identity.credentials` and `identity.sessions` collections. Passwords are never
stored: each credential uses a unique random salt and an scrypt hash. Session
tokens are random, stored only by SHA-256 digest, expire after eight hours, and
can be revoked by logout.

Public sign-up always creates the `MEMBER` role in `organization-1`. Role and
tenant values are returned from server-owned session records; the client cannot
self-assign administrator access. Protected reservation commands derive user
and organization identity from the bearer session rather than request headers.

## Consequences

This is intentionally smaller than a complete identity platform: password
reset, verification, MFA, federation, lockout, and operator role-management
APIs remain out of scope. ADR-007 remains the target for an Internet-facing
deployment; migrating to it must preserve the neutral application identity
boundary and remove local password handling.

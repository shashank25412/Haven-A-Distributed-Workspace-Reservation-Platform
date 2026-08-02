/**
 * @file idempotency_scope.hpp
 * @brief Defines the complete identity scope of an idempotent operation.
 */

#pragma once

#include "haven/domain/value_objects/idempotency_key.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/user_id.hpp"

#include <utility>

namespace haven::application::idempotency {

/** @brief Identifies the application operation protected by an idempotency key. */
enum class IdempotencyOperation { CreateReservation, ExtendReservation };

/**
 * @brief Scopes a caller key by organization, authenticated user, and operation.
 *
 * Raw scope values remain unmodified here. Infrastructure is responsible for
 * producing a bounded, safe persistence key later.
 */
class IdempotencyScope final {
public:
    IdempotencyScope(haven::domain::OrganizationId organization_id,
                     haven::domain::UserId creator_id,
                     IdempotencyOperation operation,
                     haven::domain::IdempotencyKey key)
        : organization_id_(std::move(organization_id)),
          creator_id_(std::move(creator_id)),
          operation_(operation),
          key_(std::move(key)) {}

    [[nodiscard]] const haven::domain::OrganizationId& organization_id() const noexcept {
        return organization_id_;
    }

    [[nodiscard]] const haven::domain::UserId& creator_id() const noexcept {
        return creator_id_;
    }

    [[nodiscard]] IdempotencyOperation operation() const noexcept {
        return operation_;
    }

    [[nodiscard]] const haven::domain::IdempotencyKey& key() const noexcept {
        return key_;
    }

    bool operator==(const IdempotencyScope&) const = default;

private:
    haven::domain::OrganizationId organization_id_;
    haven::domain::UserId creator_id_;
    IdempotencyOperation operation_;
    haven::domain::IdempotencyKey key_;
};

}  // namespace haven::application::idempotency

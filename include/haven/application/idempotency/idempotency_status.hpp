/**
 * @file idempotency_status.hpp
 * @brief Defines application-level idempotent operation states.
 */

#pragma once

namespace haven::application::idempotency {

/**
 * @brief Represents the durable logical state of an idempotent operation.
 *
 * Transient infrastructure failures are not terminal states. Expiry is a
 * persistence policy rather than an operation lifecycle state.
 */
enum class IdempotencyStatus { Processing, Succeeded, FailedPermanent };

}  // namespace haven::application::idempotency

/**
 * @file idempotency_key.hpp
 * @brief Defines the idempotency key domain value object.
 */

#pragma once

#include <compare>
#include <string>

namespace haven::domain {

/**
 * @brief Identifies an idempotent application operation.
 *
 * Idempotency keys are opaque client-generated values. Haven preserves the
 * supplied value exactly and combines it with the organization, user, and
 * operation when constructing the durable idempotency scope.
 *
 * Idempotency keys may contain sensitive correlation information and should
 * not be written directly to logs.
 */
class IdempotencyKey final {
public:
    /**
     * @brief Constructs an idempotency key.
     *
     * @param value Opaque client-generated key value.
     *
     * @throws std::invalid_argument when the supplied value is empty.
     */
    explicit IdempotencyKey(std::string value);

    /**
     * @brief Returns the underlying opaque key value.
     *
     * @return Reference to the stored key.
     */
    [[nodiscard]] const std::string& value() const noexcept;

    auto operator<=>(const IdempotencyKey&) const = default;

private:
    std::string value_;
};

}  // namespace haven::domain
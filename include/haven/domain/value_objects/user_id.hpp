/**
 * @file user_id.hpp
 * @brief Defines the user identifier domain value object.
 */

#pragma once

#include <compare>
#include <string>

namespace haven::domain {

/**
 * @brief Identifies an authenticated user within Haven.
 *
 * User identifiers are opaque values obtained from a trusted identity source.
 * Construction rejects an empty identifier without enforcing a specific
 * external identity format.
 */
class UserId final {
public:
    /**
     * @brief Constructs a user identifier.
     *
     * @param value Opaque user identifier value.
     *
     * @throws std::invalid_argument when the supplied value is empty.
     */
    explicit UserId(std::string value);

    /**
     * @brief Returns the underlying opaque identifier value.
     *
     * @return Reference to the stored identifier.
     */
    [[nodiscard]] const std::string& value() const noexcept;

    auto operator<=>(const UserId&) const = default;

private:
    std::string value_;
};

}  // namespace haven::domain
/**
 * @file purpose.hpp
 * @brief Defines the reservation purpose domain value object.
 */

#pragma once

#include <compare>
#include <string>

namespace haven::domain {

/**
 * @brief Stores the optional human-readable context for a reservation.
 *
 * Purpose is free-form text and may be empty or contain only whitespace.
 * The supplied value is preserved exactly without normalization.
 */
class Purpose final {
public:
    /**
     * @brief Constructs a reservation purpose.
     *
     * @param value Free-form reservation purpose.
     */
    explicit Purpose(std::string value);

    /**
     * @brief Returns the stored purpose text.
     *
     * @return Reference to the original purpose value.
     */
    [[nodiscard]] const std::string& value() const noexcept;

    auto operator<=>(const Purpose&) const = default;

private:
    std::string value_;
};

}  // namespace haven::domain
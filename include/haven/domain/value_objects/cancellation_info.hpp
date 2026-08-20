/**
 * @file cancellation_info.hpp
 * @brief Defines the reservation cancellation information domain value object.
 */

#pragma once

#include "haven/domain/value_objects/user_id.hpp"

#include <chrono>
#include <compare>
#include <optional>
#include <string>

namespace haven::domain {

/**
 * @brief Records the cancellation of a pending or confirmed reservation.
 *
 * Cancellation information is created whenever a reservation is cancelled,
 * whether by its creator or by an administrator on their behalf. A free-form
 * reason is optional; cancellers are not required to provide one.
 */
class CancellationInfo final {
public:
    using TimePoint = std::chrono::system_clock::time_point;

    /**
     * @brief Constructs reservation cancellation information.
     *
     * @param cancelled_by Identifier of the user who cancelled the reservation.
     * @param cancelled_at Time at which the cancellation occurred.
     * @param reason Optional free-form explanation for the cancellation.
     */
    CancellationInfo(UserId cancelled_by,
                     TimePoint cancelled_at,
                     std::optional<std::string> reason = std::nullopt);

    /**
     * @brief Returns the identifier of the cancelling user.
     */
    [[nodiscard]] const UserId& cancelled_by() const noexcept;

    /**
     * @brief Returns the time at which cancellation occurred.
     */
    [[nodiscard]] TimePoint cancelled_at() const noexcept;

    /**
     * @brief Returns the optional cancellation reason.
     */
    [[nodiscard]] const std::optional<std::string>& reason() const noexcept;

    auto operator<=>(const CancellationInfo&) const = default;

private:
    UserId cancelled_by_;
    TimePoint cancelled_at_;
    std::optional<std::string> reason_;
};

}  // namespace haven::domain

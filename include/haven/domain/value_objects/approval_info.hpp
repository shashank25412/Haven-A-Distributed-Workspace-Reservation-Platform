/**
 * @file approval_info.hpp
 * @brief Defines the reservation approval information domain value object.
 */

#pragma once

#include "haven/domain/value_objects/user_id.hpp"

#include <chrono>
#include <compare>

namespace haven::domain {

/**
 * @brief Records the successful approval of a reservation.
 *
 * Approval information is created only after a pending reservation passes the
 * authoritative conflict check and transitions to confirmed. Rejection details
 * are not represented by this type.
 */
class ApprovalInfo final {
public:
    using TimePoint = std::chrono::system_clock::time_point;

    /**
     * @brief Constructs reservation approval information.
     *
     * @param approved_by Identifier of the user who approved the reservation.
     * @param approved_at Time at which the approval occurred.
     */
    ApprovalInfo(UserId approved_by, TimePoint approved_at);

    /**
     * @brief Returns the identifier of the approving user.
     *
     * @return Approving user identifier.
     */
    [[nodiscard]] const UserId& approved_by() const noexcept;

    /**
     * @brief Returns the time at which approval occurred.
     *
     * @return Approval timestamp.
     */
    [[nodiscard]] TimePoint approved_at() const noexcept;

    auto operator<=>(const ApprovalInfo&) const = default;

private:
    UserId approved_by_;
    TimePoint approved_at_;
};

}  // namespace haven::domain
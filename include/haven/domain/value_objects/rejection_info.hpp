/**
 * @file rejection_info.hpp
 * @brief Defines the reservation rejection information domain value object.
 */

#pragma once

#include "haven/domain/value_objects/user_id.hpp"

#include <chrono>
#include <compare>
#include <optional>
#include <string>

namespace haven::domain {

/**
 * @brief Records the rejection of a pending reservation.
 *
 * Rejection information is created only when a pending reservation is denied
 * by an approver. A free-form reason is optional; approvers are not required
 * to provide one.
 */
class RejectionInfo final {
public:
    using TimePoint = std::chrono::system_clock::time_point;

    /**
     * @brief Constructs reservation rejection information.
     *
     * @param rejected_by Identifier of the user who rejected the reservation.
     * @param rejected_at Time at which the rejection occurred.
     * @param reason Optional free-form explanation for the rejection.
     */
    RejectionInfo(UserId rejected_by, TimePoint rejected_at, std::optional<std::string> reason = std::nullopt);

    /**
     * @brief Returns the identifier of the rejecting user.
     */
    [[nodiscard]] const UserId& rejected_by() const noexcept;

    /**
     * @brief Returns the time at which rejection occurred.
     */
    [[nodiscard]] TimePoint rejected_at() const noexcept;

    /**
     * @brief Returns the optional rejection reason.
     */
    [[nodiscard]] const std::optional<std::string>& reason() const noexcept;

    auto operator<=>(const RejectionInfo&) const = default;

private:
    UserId rejected_by_;
    TimePoint rejected_at_;
    std::optional<std::string> reason_;
};

}  // namespace haven::domain

/**
 * @file cancellation_info.cpp
 * @brief Implements the reservation cancellation information domain value object.
 */

#include "haven/domain/value_objects/cancellation_info.hpp"

#include <utility>

namespace haven::domain {

CancellationInfo::CancellationInfo(UserId cancelled_by,
                                   const TimePoint cancelled_at,
                                   std::optional<std::string> reason)
    : cancelled_by_(std::move(cancelled_by)),
      cancelled_at_(cancelled_at),
      reason_(std::move(reason)) {}

const UserId& CancellationInfo::cancelled_by() const noexcept {
    return cancelled_by_;
}

CancellationInfo::TimePoint CancellationInfo::cancelled_at() const noexcept {
    return cancelled_at_;
}

const std::optional<std::string>& CancellationInfo::reason() const noexcept {
    return reason_;
}

}  // namespace haven::domain

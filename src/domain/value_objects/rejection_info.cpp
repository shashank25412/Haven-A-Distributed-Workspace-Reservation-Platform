/**
 * @file rejection_info.cpp
 * @brief Implements the reservation rejection information domain value object.
 */

#include "haven/domain/value_objects/rejection_info.hpp"

#include <utility>

namespace haven::domain {

RejectionInfo::RejectionInfo(UserId rejected_by,
                             const TimePoint rejected_at,
                             std::optional<std::string> reason)
    : rejected_by_(std::move(rejected_by)), rejected_at_(rejected_at), reason_(std::move(reason)) {
}

const UserId& RejectionInfo::rejected_by() const noexcept {
    return rejected_by_;
}

RejectionInfo::TimePoint RejectionInfo::rejected_at() const noexcept {
    return rejected_at_;
}

const std::optional<std::string>& RejectionInfo::reason() const noexcept {
    return reason_;
}

}  // namespace haven::domain

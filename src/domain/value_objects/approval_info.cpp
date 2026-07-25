/**
 * @file approval_info.cpp
 * @brief Implements the reservation approval information domain value object.
 */

#include "haven/domain/value_objects/approval_info.hpp"

#include <utility>

namespace haven::domain {

ApprovalInfo::ApprovalInfo(UserId approved_by, const TimePoint approved_at)
    : approved_by_(std::move(approved_by)), approved_at_(approved_at) {
}

const UserId& ApprovalInfo::approved_by() const noexcept {
    return approved_by_;
}

ApprovalInfo::TimePoint ApprovalInfo::approved_at() const noexcept {
    return approved_at_;
}

}  // namespace haven::domain
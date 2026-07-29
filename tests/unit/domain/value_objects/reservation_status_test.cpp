/**
 * @file reservation_status_test.cpp
 * @brief Tests the supported Haven reservation states.
 */

#include "haven/domain/value_objects/reservation_status.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

namespace haven::domain {
namespace {

TEST(ReservationStatusTest, IsTerminal_ShouldReturnFalse_WhenStatusIsPendingApproval) {
    EXPECT_FALSE(is_terminal(ReservationStatus::PendingApproval));
}

TEST(ReservationStatusTest, IsTerminal_ShouldReturnFalse_WhenStatusIsConfirmed) {
    EXPECT_FALSE(is_terminal(ReservationStatus::Confirmed));
}

TEST(ReservationStatusTest, IsTerminal_ShouldReturnTrue_WhenStatusIsCancelled) {
    EXPECT_TRUE(is_terminal(ReservationStatus::Cancelled));
}

TEST(ReservationStatusTest, IsTerminal_ShouldReturnTrue_WhenStatusIsRejected) {
    EXPECT_TRUE(is_terminal(ReservationStatus::Rejected));
}

TEST(ReservationStatusTest, IsTerminal_ShouldReturnTrue_WhenStatusIsExpired) {
    EXPECT_TRUE(is_terminal(ReservationStatus::Expired));
}

TEST(ReservationStatusTest, IsTerminal_ShouldReturnTrue_WhenStatusIsCompleted) {
    EXPECT_TRUE(is_terminal(ReservationStatus::Completed));
}

TEST(ReservationStatusTest, ToString_ShouldReturnPendingApproval_WhenStatusIsPendingApproval) {
    EXPECT_EQ(to_string(ReservationStatus::PendingApproval), "PENDING_APPROVAL");
}

TEST(ReservationStatusTest, ToString_ShouldReturnConfirmed_WhenStatusIsConfirmed) {
    EXPECT_EQ(to_string(ReservationStatus::Confirmed), "CONFIRMED");
}

TEST(ReservationStatusTest, ToString_ShouldReturnCancelled_WhenStatusIsCancelled) {
    EXPECT_EQ(to_string(ReservationStatus::Cancelled), "CANCELLED");
}

TEST(ReservationStatusTest, ToString_ShouldReturnRejected_WhenStatusIsRejected) {
    EXPECT_EQ(to_string(ReservationStatus::Rejected), "REJECTED");
}

TEST(ReservationStatusTest, ToString_ShouldReturnExpired_WhenStatusIsExpired) {
    EXPECT_EQ(to_string(ReservationStatus::Expired), "EXPIRED");
}

TEST(ReservationStatusTest, ToString_ShouldReturnCompleted_WhenStatusIsCompleted) {
    EXPECT_EQ(to_string(ReservationStatus::Completed), "COMPLETED");
}

TEST(ReservationStatusTest, ToString_ShouldReturnUnknown_WhenStatusIsUnsupported) {
    const auto unsupported_status = static_cast<ReservationStatus>(999);

    EXPECT_EQ(to_string(unsupported_status), "UNKNOWN");
}

TEST(ReservationStatusTest, FromString_ShouldReturnEverySupportedStatus) {
    EXPECT_EQ(reservation_status_from_string("PENDING_APPROVAL"),
              ReservationStatus::PendingApproval);
    EXPECT_EQ(reservation_status_from_string("CONFIRMED"), ReservationStatus::Confirmed);
    EXPECT_EQ(reservation_status_from_string("CANCELLED"), ReservationStatus::Cancelled);
    EXPECT_EQ(reservation_status_from_string("REJECTED"), ReservationStatus::Rejected);
    EXPECT_EQ(reservation_status_from_string("EXPIRED"), ReservationStatus::Expired);
    EXPECT_EQ(reservation_status_from_string("COMPLETED"), ReservationStatus::Completed);
}

TEST(ReservationStatusTest, FromString_ShouldRejectUnsupportedStatus) {
    EXPECT_THROW(static_cast<void>(reservation_status_from_string("UNKNOWN")),
                 std::invalid_argument);
}

}  // namespace
}  // namespace haven::domain

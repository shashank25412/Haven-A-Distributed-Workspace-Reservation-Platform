#include "haven/domain/reservation.hpp"
#include "haven/presentation/reservations/create_reservation_request.hpp"
#include "haven/presentation/reservations/create_reservation_response.hpp"

#include <gtest/gtest.h>

namespace haven::presentation::reservations {
namespace {

Json::Value valid_request() {
    Json::Value json;
    json["resourceId"] = "resource-1";
    json["startTime"] = "2026-08-01T10:00:00Z";
    json["endTime"] = "2026-08-01T11:00:00Z";
    json["purpose"] = "  planning  ";
    return json;
}

TEST(CreateReservationRequestTest, ParsesContractAndPreservesPurposeWhitespace) {
    const auto request = CreateReservationRequest::from_json(valid_request());
    EXPECT_EQ(request.resource_id().value(), "resource-1");
    EXPECT_EQ(request.purpose().value(), "  planning  ");
    EXPECT_LT(request.interval().start(), request.interval().end());
}

TEST(CreateReservationRequestTest, DefaultsMissingPurposeToEmpty) {
    auto json = valid_request();
    json.removeMember("purpose");
    EXPECT_EQ(CreateReservationRequest::from_json(json).purpose().value(), "");
}

TEST(CreateReservationRequestTest, RejectsMissingMalformedAndReversedFields) {
    auto missing = valid_request();
    missing.removeMember("resourceId");
    EXPECT_THROW(static_cast<void>(CreateReservationRequest::from_json(missing)),
                 std::invalid_argument);
    auto malformed = valid_request();
    malformed["startTime"] = "not-a-time";
    EXPECT_THROW(static_cast<void>(CreateReservationRequest::from_json(malformed)),
                 std::invalid_argument);
    auto reversed = valid_request();
    reversed["endTime"] = reversed["startTime"];
    EXPECT_THROW(static_cast<void>(CreateReservationRequest::from_json(reversed)),
                 std::invalid_argument);
}

TEST(CreateReservationRequestTest, RejectsInvalidResourceIdentifiers) {
    auto request = valid_request();
    request["resourceId"] = "   ";
    EXPECT_THROW((void)CreateReservationRequest::from_json(request), std::invalid_argument);

    request["resourceId"] = std::string(256, 'r');
    EXPECT_THROW((void)CreateReservationRequest::from_json(request), std::invalid_argument);
}

TEST(CreateReservationResponseTest, SerializesOnlyOpenApiSuccessFields) {
    const auto created_at =
        std::chrono::system_clock::time_point{std::chrono::seconds{1'785'573'000}};
    auto reservation = haven::domain::Reservation::create_confirmed(
        haven::domain::OrganizationId{"organization-1"},
        haven::domain::ReservationId{"reservation-1"},
        haven::domain::ResourceId{"resource-1"},
        haven::domain::UserId{"user-1"},
        haven::domain::TimeInterval{created_at + std::chrono::hours{1},
                                    created_at + std::chrono::hours{2}},
        haven::domain::Purpose{"secret purpose"},
        haven::domain::ReservationKind::Standard,
        haven::domain::EventId{"created-event"},
        haven::domain::EventId{"confirmed-event"},
        created_at);
    const auto result = haven::application::reservations::CreateReservationResult::confirmed(
        std::move(reservation), created_at);
    const auto json = CreateReservationResponse{result}.to_json();
    EXPECT_EQ(json.size(), 6U);
    EXPECT_EQ(json["status"].asString(), "CONFIRMED");
    EXPECT_TRUE(json["createdAt"].asString().ends_with("Z"));
    EXPECT_FALSE(json.isMember("purpose"));
    EXPECT_FALSE(json.isMember("version"));
    EXPECT_FALSE(json.isMember("idempotencyKey"));
    EXPECT_FALSE(json.isMember("fingerprint"));
    EXPECT_FALSE(json.isMember("eventId"));
    EXPECT_FALSE(json.isMember("cas"));
}

}  // namespace
}  // namespace haven::presentation::reservations

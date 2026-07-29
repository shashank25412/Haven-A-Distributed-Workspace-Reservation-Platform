/**
 * @file test_reservation_repository_test.cpp
 * @brief Verifies the concurrency-aware application test repository.
 */

#include "application/util/test_reservation_repository.hpp"

#include "haven/application/repository_error.hpp"

#include <gtest/gtest.h>

#include <chrono>

namespace haven::tests::util::application {
namespace {

[[nodiscard]] haven::domain::Reservation make_reservation() {
    using namespace std::chrono_literals;
    const auto start = haven::domain::Reservation::TimePoint{} + 1h;
    return haven::domain::Reservation::create_confirmed(
        haven::domain::OrganizationId{"organization-1"},
        haven::domain::ReservationId{"reservation-1"},
        haven::domain::ResourceId{"resource-1"},
        haven::domain::UserId{"user-1"},
        haven::domain::TimeInterval{start, start + 1h},
        haven::domain::Purpose{"test"},
        haven::domain::ReservationKind::Standard,
        haven::domain::EventId{"created-1"},
        haven::domain::EventId{"confirmed-1"},
        start);
}

TEST(TestReservationRepositoryTest, InsertFindAndUpdateRotateOpaqueTokens) {
    auto repository = TestReservationRepository{};
    const auto organization_id = haven::domain::OrganizationId{"organization-1"};
    auto reservation = make_reservation();

    const auto inserted_token = repository.insert(organization_id, reservation);
    const auto loaded =
        repository.find_by_id(organization_id, haven::domain::ReservationId{"reservation-1"});
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->persistence_token(), inserted_token);

    reservation.complete(haven::domain::Reservation::TimePoint{} + std::chrono::hours{3},
                         haven::domain::EventId{"completed-1"});
    const auto updated_token =
        repository.update(organization_id, reservation, loaded->persistence_token());

    EXPECT_NE(updated_token, inserted_token);
    EXPECT_EQ(repository.insert_call_count(), 1U);
    EXPECT_EQ(repository.update_call_count(), 1U);
    EXPECT_EQ(repository.last_expected_token(), inserted_token);
}

TEST(TestReservationRepositoryTest, StaleUpdateThrowsConcurrencyConflictWithoutRetry) {
    auto repository = TestReservationRepository{};
    const auto organization_id = haven::domain::OrganizationId{"organization-1"};
    const auto reservation = make_reservation();
    static_cast<void>(repository.insert(organization_id, reservation));

    try {
        static_cast<void>(repository.update(
            organization_id, reservation, haven::application::persistence::PersistenceToken{999}));
        FAIL() << "Expected concurrency conflict";
    } catch (const haven::application::RepositoryError& error) {
        EXPECT_EQ(error.code(), haven::application::RepositoryErrorCode::ConcurrencyConflict);
    }
    EXPECT_EQ(repository.update_call_count(), 1U);
}

TEST(TestReservationRepositoryTest, DuplicateInsertThrowsAlreadyExists) {
    auto repository = TestReservationRepository{};
    const auto organization_id = haven::domain::OrganizationId{"organization-1"};
    const auto reservation = make_reservation();
    static_cast<void>(repository.insert(organization_id, reservation));

    try {
        static_cast<void>(repository.insert(organization_id, reservation));
        FAIL() << "Expected duplicate insert failure";
    } catch (const haven::application::RepositoryError& error) {
        EXPECT_EQ(error.code(), haven::application::RepositoryErrorCode::AlreadyExists);
    }
}

}  // namespace
}  // namespace haven::tests::util::application

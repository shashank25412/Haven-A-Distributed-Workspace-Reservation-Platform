/**
 * @file couchbase_reservation_repository_test.cpp
 * @brief Exercises the reservation repository against a live Couchbase service.
 */

#include "haven/infrastructure/persistence/couchbase/couchbase_reservation_repository.hpp"

#include "haven/application/repository_error.hpp"
#include "haven/domain/value_objects/approval_info.hpp"
#include "haven/domain/value_objects/purpose.hpp"
#include "haven/domain/value_objects/reservation_kind.hpp"
#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/domain/value_objects/version.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_collections.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_configuration.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_document_key.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <couchbase/error_codes.hxx>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

namespace persistence = haven::infrastructure::persistence::couchbase;
using TimePoint = haven::domain::Reservation::TimePoint;

[[nodiscard]] std::optional<std::string> environment_value(const char* name) {
    const auto* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return std::nullopt;
    }
    return std::string{value};
}

[[nodiscard]] std::optional<persistence::CouchbaseConfiguration> integration_configuration() {
    const auto connection_string = environment_value("HVN_COUCHBASE_CONNECTION_STRING");
    const auto username = environment_value("HVN_COUCHBASE_USERNAME");
    const auto password = environment_value("HVN_COUCHBASE_PASSWORD");
    const auto bucket = environment_value("HVN_COUCHBASE_BUCKET");
    const auto scope = environment_value("HVN_COUCHBASE_SCOPE");
    if (!connection_string || !username || !password || !bucket || !scope) {
        return std::nullopt;
    }
    return persistence::CouchbaseConfiguration{
        .connection_string = *connection_string,
        .username = *username,
        .password = *password,
        .bucket_name = *bucket,
        .scope_name = *scope,
    };
}

[[nodiscard]] std::string unique_suffix() {
    static std::uint64_t sequence{0};
    ++sequence;
    return std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" +
           std::to_string(sequence);
}

[[nodiscard]] TimePoint at(const std::int64_t seconds) {
    return TimePoint{std::chrono::seconds{1'800'000'000 + seconds}};
}

class CouchbaseReservationRepositoryIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto configuration = integration_configuration();
        if (!configuration) {
            GTEST_SKIP() << "Set all HVN_COUCHBASE_* variables to run Couchbase integration tests";
        }
        connection_ = std::make_shared<persistence::CouchbaseConnection>(*configuration);
        repository_ = std::make_unique<persistence::CouchbaseReservationRepository>(connection_);
    }

    void TearDown() override {
        if (!connection_) {
            return;
        }
        auto collection = connection_->collection(persistence::CouchbaseCollections::reservations);
        for (const auto& key : keys_) {
            auto [error, result] = collection.remove(key).get();
            static_cast<void>(result);
            if (error && error.ec() != ::couchbase::errc::key_value::document_not_found) {
                ADD_FAILURE() << "Failed to clean up " << key << ": " << error.ec().message();
            }
        }
    }

    [[nodiscard]] haven::domain::Reservation make_reservation(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ReservationId& reservation_id,
        const haven::domain::ResourceId& resource_id,
        const haven::domain::UserId& creator,
        const haven::domain::TimeInterval& interval,
        std::string purpose,
        const haven::domain::ReservationStatus status = haven::domain::ReservationStatus::Confirmed,
        const haven::domain::ReservationKind kind = haven::domain::ReservationKind::Standard,
        std::optional<haven::domain::ApprovalInfo> approval = std::nullopt,
        const std::uint64_t version = 1) {
        register_key(organization_id, reservation_id);
        return haven::domain::Reservation::rehydrate(organization_id,
                                                     reservation_id,
                                                     resource_id,
                                                     creator,
                                                     interval,
                                                     haven::domain::Purpose{std::move(purpose)},
                                                     kind,
                                                     status,
                                                     std::move(approval),
                                                     haven::domain::Version{version});
    }

    [[nodiscard]] haven::domain::Reservation make_pending_reservation(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ReservationId& reservation_id,
        const haven::domain::ResourceId& resource_id,
        const haven::domain::UserId& creator,
        const haven::domain::TimeInterval& interval,
        std::string purpose) {
        register_key(organization_id, reservation_id);
        return haven::domain::Reservation::create_pending_approval(
            organization_id,
            reservation_id,
            resource_id,
            creator,
            interval,
            haven::domain::Purpose{std::move(purpose)},
            haven::domain::ReservationKind::Standard,
            haven::domain::EventId{"created-" + unique_suffix()},
            haven::domain::EventId{"approval-requested-" + unique_suffix()},
            at(1));
    }

    void register_key(const haven::domain::OrganizationId& organization_id,
                      const haven::domain::ReservationId& reservation_id) {
        const auto key = persistence::reservation_document_key(organization_id, reservation_id);
        if (std::find(keys_.begin(), keys_.end(), key) == keys_.end()) {
            keys_.push_back(key);
        }
    }

    std::shared_ptr<persistence::CouchbaseConnection> connection_;
    std::unique_ptr<persistence::CouchbaseReservationRepository> repository_;
    std::vector<std::string> keys_;
};

TEST_F(CouchbaseReservationRepositoryIntegrationTest, SavesAndRestoresEveryPersistedField) {
    const auto suffix = unique_suffix();
    const auto organization = haven::domain::OrganizationId{"organization-" + suffix};
    const auto id = haven::domain::ReservationId{"reservation-" + suffix};
    const auto resource = haven::domain::ResourceId{"resource-" + suffix};
    const auto creator = haven::domain::UserId{"creator-" + suffix};
    const auto interval = haven::domain::TimeInterval{at(100), at(200)};
    const auto approver = haven::domain::UserId{"approver-" + suffix};
    const auto reservation = make_reservation(organization,
                                              id,
                                              resource,
                                              creator,
                                              interval,
                                              "  Architecture review  ",
                                              haven::domain::ReservationStatus::Confirmed,
                                              haven::domain::ReservationKind::Maintenance,
                                              haven::domain::ApprovalInfo{approver, at(50)},
                                              7);

    static_cast<void>(repository_->insert(organization, reservation));
    const auto found = repository_->find_by_id(organization, id);

    ASSERT_TRUE(found);
    EXPECT_EQ(found->aggregate().organization_id(), organization);
    EXPECT_EQ(found->aggregate().reservation_id(), id);
    EXPECT_EQ(found->aggregate().resource_id(), resource);
    EXPECT_EQ(found->aggregate().created_by(), creator);
    EXPECT_EQ(found->aggregate().interval(), interval);
    EXPECT_EQ(found->aggregate().purpose().value(), "  Architecture review  ");
    EXPECT_EQ(found->aggregate().status(), haven::domain::ReservationStatus::Confirmed);
    EXPECT_EQ(found->aggregate().kind(), haven::domain::ReservationKind::Maintenance);
    ASSERT_TRUE(found->aggregate().approval_info());
    EXPECT_EQ(found->aggregate().approval_info()->approved_by(), approver);
    EXPECT_EQ(found->aggregate().approval_info()->approved_at(), at(50));
    EXPECT_EQ(found->aggregate().version().value(), 7U);
}

TEST_F(CouchbaseReservationRepositoryIntegrationTest, PreservesPendingAndEmptyPurpose) {
    const auto suffix = unique_suffix();
    const auto organization = haven::domain::OrganizationId{"organization-" + suffix};
    const auto id = haven::domain::ReservationId{"reservation-" + suffix};
    const auto reservation = make_reservation(organization,
                                              id,
                                              haven::domain::ResourceId{"resource-" + suffix},
                                              haven::domain::UserId{"creator-" + suffix},
                                              haven::domain::TimeInterval{at(300), at(400)},
                                              "",
                                              haven::domain::ReservationStatus::PendingApproval);
    static_cast<void>(repository_->insert(organization, reservation));

    const auto found = repository_->find_by_id(organization, id);
    ASSERT_TRUE(found);
    EXPECT_EQ(found->aggregate().status(), haven::domain::ReservationStatus::PendingApproval);
    EXPECT_TRUE(found->aggregate().purpose().value().empty());
}

TEST_F(CouchbaseReservationRepositoryIntegrationTest, MissingAndWrongOrganizationReturnEmpty) {
    const auto suffix = unique_suffix();
    const auto owner = haven::domain::OrganizationId{"owner-" + suffix};
    const auto other = haven::domain::OrganizationId{"other-" + suffix};
    const auto id = haven::domain::ReservationId{"reservation-" + suffix};
    auto reservation = make_reservation(owner,
                                        id,
                                        haven::domain::ResourceId{"resource-" + suffix},
                                        haven::domain::UserId{"creator-" + suffix},
                                        haven::domain::TimeInterval{at(500), at(600)},
                                        "purpose");
    static_cast<void>(repository_->insert(owner, reservation));

    EXPECT_FALSE(repository_->find_by_id(other, id));
    EXPECT_FALSE(repository_->find_by_id(owner, haven::domain::ReservationId{"missing-" + suffix}));
}

TEST_F(CouchbaseReservationRepositoryIntegrationTest,
       SameReservationIdIsIsolatedAcrossOrganizations) {
    const auto suffix = unique_suffix();
    const auto first_organization = haven::domain::OrganizationId{"first-" + suffix};
    const auto second_organization = haven::domain::OrganizationId{"second-" + suffix};
    const auto reservation_id = haven::domain::ReservationId{"shared-" + suffix};
    const auto resource_id = haven::domain::ResourceId{"resource-" + suffix};
    const auto creator = haven::domain::UserId{"creator-" + suffix};

    auto first = make_reservation(first_organization,
                                  reservation_id,
                                  resource_id,
                                  creator,
                                  haven::domain::TimeInterval{at(610), at(620)},
                                  "first");
    auto second = make_reservation(second_organization,
                                   reservation_id,
                                   resource_id,
                                   creator,
                                   haven::domain::TimeInterval{at(630), at(640)},
                                   "second");
    static_cast<void>(repository_->insert(first_organization, first));
    static_cast<void>(repository_->insert(second_organization, second));

    const auto first_found = repository_->find_by_id(first_organization, reservation_id);
    const auto second_found = repository_->find_by_id(second_organization, reservation_id);

    ASSERT_TRUE(first_found);
    ASSERT_TRUE(second_found);
    EXPECT_EQ(first_found->aggregate().purpose().value(), "first");
    EXPECT_EQ(second_found->aggregate().purpose().value(), "second");
}

TEST_F(CouchbaseReservationRepositoryIntegrationTest, SaveReplacesExistingLifecycleState) {
    const auto suffix = unique_suffix();
    const auto organization = haven::domain::OrganizationId{"organization-" + suffix};
    const auto id = haven::domain::ReservationId{"reservation-" + suffix};
    auto first = make_reservation(organization,
                                  id,
                                  haven::domain::ResourceId{"resource-" + suffix},
                                  haven::domain::UserId{"creator-" + suffix},
                                  haven::domain::TimeInterval{at(700), at(800)},
                                  "first",
                                  haven::domain::ReservationStatus::PendingApproval);
    const auto token = repository_->insert(organization, first);
    auto updated = make_reservation(organization,
                                    id,
                                    first.resource_id(),
                                    first.created_by(),
                                    first.interval(),
                                    "updated",
                                    haven::domain::ReservationStatus::Confirmed,
                                    haven::domain::ReservationKind::Standard,
                                    std::nullopt,
                                    2);
    static_cast<void>(repository_->update(organization, updated, token));

    const auto found = repository_->find_by_id(organization, id);
    ASSERT_TRUE(found);
    EXPECT_EQ(found->aggregate().purpose().value(), "updated");
    EXPECT_EQ(found->aggregate().status(), haven::domain::ReservationStatus::Confirmed);
    EXPECT_EQ(found->aggregate().version().value(), 2U);
}

TEST_F(CouchbaseReservationRepositoryIntegrationTest,
       DuplicateInsertIsAlreadyExistsAndReturnsUsableToken) {
    const auto suffix = unique_suffix();
    const auto organization = haven::domain::OrganizationId{"organization-" + suffix};
    const auto id = haven::domain::ReservationId{"reservation-" + suffix};
    const auto reservation = make_pending_reservation(
        organization,
        id,
        haven::domain::ResourceId{"resource-" + suffix},
        haven::domain::UserId{"creator-" + suffix},
        haven::domain::TimeInterval{at(2'000), at(2'100)},
        "duplicate");

    const auto inserted_token = repository_->insert(organization, reservation);
    const auto loaded = repository_->find_by_id(organization, id);
    ASSERT_TRUE(loaded);
    EXPECT_EQ(loaded->persistence_token(), inserted_token);
    EXPECT_EQ(loaded->aggregate().version(), haven::domain::Version{1});

    try {
        static_cast<void>(repository_->insert(organization, reservation));
        FAIL() << "Expected duplicate insert to fail";
    } catch (const haven::application::RepositoryError& error) {
        EXPECT_EQ(error.code(), haven::application::RepositoryErrorCode::AlreadyExists);
        EXPECT_NE(error.code(), haven::application::RepositoryErrorCode::ConcurrencyConflict);
    }
}

TEST_F(CouchbaseReservationRepositoryIntegrationTest,
       StaleTokenCannotOverwriteNewerReservationState) {
    const auto suffix = unique_suffix();
    const auto organization = haven::domain::OrganizationId{"organization-" + suffix};
    const auto id = haven::domain::ReservationId{"reservation-" + suffix};
    const auto approver = haven::domain::UserId{"approver-" + suffix};
    const auto reservation = make_pending_reservation(
        organization,
        id,
        haven::domain::ResourceId{"resource-" + suffix},
        haven::domain::UserId{"creator-" + suffix},
        haven::domain::TimeInterval{at(2'200), at(2'300)},
        "stale-write");
    EXPECT_EQ(reservation.version(), haven::domain::Version{1});
    static_cast<void>(repository_->insert(organization, reservation));

    auto copy_a = repository_->find_by_id(organization, id);
    auto copy_b = repository_->find_by_id(organization, id);
    ASSERT_TRUE(copy_a);
    ASSERT_TRUE(copy_b);
    EXPECT_EQ(copy_a->persistence_token(), copy_b->persistence_token());

    copy_a->aggregate().approve(
        approver, at(2'150), haven::domain::EventId{"confirmed-" + suffix});
    EXPECT_EQ(copy_a->aggregate().version(), haven::domain::Version{2});
    const auto token_a2 = repository_->update(
        organization, copy_a->aggregate(), copy_a->persistence_token());
    EXPECT_NE(token_a2, copy_a->persistence_token());

    copy_b->aggregate().reject(
        approver, at(2'160), haven::domain::EventId{"rejected-" + suffix});
    EXPECT_EQ(copy_b->aggregate().version(), haven::domain::Version{2});
    try {
        static_cast<void>(repository_->update(
            organization, copy_b->aggregate(), copy_b->persistence_token()));
        FAIL() << "Expected stale update to fail";
    } catch (const haven::application::RepositoryError& error) {
        EXPECT_EQ(error.code(), haven::application::RepositoryErrorCode::ConcurrencyConflict);
    }

    const auto reloaded = repository_->find_by_id(organization, id);
    ASSERT_TRUE(reloaded);
    EXPECT_EQ(reloaded->aggregate().status(), haven::domain::ReservationStatus::Confirmed);
    EXPECT_EQ(reloaded->aggregate().version(), haven::domain::Version{2});
    ASSERT_TRUE(reloaded->aggregate().approval_info());
    EXPECT_EQ(reloaded->aggregate().approval_info()->approved_by(), approver);
    EXPECT_EQ(reloaded->persistence_token(), token_a2);
}

TEST_F(CouchbaseReservationRepositoryIntegrationTest,
       RotatedTokenSupportsASecondIndependentDomainUpdate) {
    const auto suffix = unique_suffix();
    const auto organization = haven::domain::OrganizationId{"organization-" + suffix};
    const auto id = haven::domain::ReservationId{"reservation-" + suffix};
    const auto actor = haven::domain::UserId{"actor-" + suffix};
    const auto reservation = make_pending_reservation(
        organization,
        id,
        haven::domain::ResourceId{"resource-" + suffix},
        haven::domain::UserId{"creator-" + suffix},
        haven::domain::TimeInterval{at(2'400), at(2'500)},
        "two-updates");
    static_cast<void>(repository_->insert(organization, reservation));

    auto loaded = repository_->find_by_id(organization, id);
    ASSERT_TRUE(loaded);
    EXPECT_EQ(loaded->aggregate().version(), haven::domain::Version{1});
    loaded->aggregate().approve(actor, at(2'350), haven::domain::EventId{"confirmed-" + suffix});
    EXPECT_EQ(loaded->aggregate().version(), haven::domain::Version{2});
    const auto second_token =
        repository_->update(organization, loaded->aggregate(), loaded->persistence_token());
    EXPECT_NE(second_token, loaded->persistence_token());

    loaded->aggregate().extend(
        at(2'600), actor, at(2'360), haven::domain::EventId{"extended-" + suffix});
    EXPECT_EQ(loaded->aggregate().version(), haven::domain::Version{3});
    const auto third_token = repository_->update(organization, loaded->aggregate(), second_token);
    EXPECT_NE(third_token, second_token);

    const auto reloaded = repository_->find_by_id(organization, id);
    ASSERT_TRUE(reloaded);
    EXPECT_EQ(reloaded->aggregate().status(), haven::domain::ReservationStatus::Confirmed);
    EXPECT_EQ(reloaded->aggregate().interval().end(), at(2'600));
    EXPECT_EQ(reloaded->aggregate().version(), haven::domain::Version{3});
    EXPECT_EQ(reloaded->persistence_token(), third_token);
}

TEST_F(CouchbaseReservationRepositoryIntegrationTest,
       MissingUpdateRemainsPersistenceFailureRatherThanConcurrencyConflict) {
    const auto suffix = unique_suffix();
    const auto organization = haven::domain::OrganizationId{"organization-" + suffix};
    const auto id = haven::domain::ReservationId{"missing-" + suffix};
    const auto reservation = make_pending_reservation(
        organization,
        id,
        haven::domain::ResourceId{"resource-" + suffix},
        haven::domain::UserId{"creator-" + suffix},
        haven::domain::TimeInterval{at(2'700), at(2'800)},
        "missing");

    EXPECT_FALSE(repository_->find_by_id(organization, id));
    try {
        static_cast<void>(repository_->update(
            organization,
            reservation,
            haven::application::persistence::PersistenceToken{1}));
        FAIL() << "Expected missing update to fail";
    } catch (const haven::application::RepositoryError& error) {
        EXPECT_EQ(error.code(), haven::application::RepositoryErrorCode::Persistence);
        EXPECT_NE(error.code(), haven::application::RepositoryErrorCode::ConcurrencyConflict);
    }
}

TEST_F(CouchbaseReservationRepositoryIntegrationTest,
       TokenFromAnotherOrganizationCannotUpdateOrOverwriteEitherDocument) {
    const auto suffix = unique_suffix();
    const auto organization_a = haven::domain::OrganizationId{"organization-a-" + suffix};
    const auto organization_b = haven::domain::OrganizationId{"organization-b-" + suffix};
    const auto id = haven::domain::ReservationId{"shared-" + suffix};
    const auto actor = haven::domain::UserId{"actor-" + suffix};
    auto reservation_a = make_pending_reservation(
        organization_a,
        id,
        haven::domain::ResourceId{"resource-a-" + suffix},
        actor,
        haven::domain::TimeInterval{at(2'900), at(3'000)},
        "organization-a");
    auto reservation_b = make_pending_reservation(
        organization_b,
        id,
        haven::domain::ResourceId{"resource-b-" + suffix},
        actor,
        haven::domain::TimeInterval{at(3'100), at(3'200)},
        "organization-b");
    static_cast<void>(repository_->insert(organization_a, reservation_a));
    static_cast<void>(repository_->insert(organization_b, reservation_b));
    auto loaded_a = repository_->find_by_id(organization_a, id);
    auto loaded_b = repository_->find_by_id(organization_b, id);
    ASSERT_TRUE(loaded_a);
    ASSERT_TRUE(loaded_b);

    loaded_b->aggregate().reject(
        actor, at(3'050), haven::domain::EventId{"rejected-" + suffix});
    try {
        static_cast<void>(
            repository_->update(organization_b, loaded_b->aggregate(), loaded_a->persistence_token()));
        FAIL() << "Expected cross-organization token update to fail";
    } catch (const haven::application::RepositoryError& error) {
        EXPECT_EQ(error.code(), haven::application::RepositoryErrorCode::ConcurrencyConflict);
    }

    const auto reloaded_a = repository_->find_by_id(organization_a, id);
    const auto reloaded_b = repository_->find_by_id(organization_b, id);
    ASSERT_TRUE(reloaded_a);
    ASSERT_TRUE(reloaded_b);
    EXPECT_EQ(reloaded_a->aggregate().status(),
              haven::domain::ReservationStatus::PendingApproval);
    EXPECT_EQ(reloaded_b->aggregate().status(),
              haven::domain::ReservationStatus::PendingApproval);
    EXPECT_EQ(reloaded_a->aggregate().version(), haven::domain::Version{1});
    EXPECT_EQ(reloaded_b->aggregate().version(), haven::domain::Version{1});
    EXPECT_EQ(reloaded_a->aggregate().purpose().value(), "organization-a");
    EXPECT_EQ(reloaded_b->aggregate().purpose().value(), "organization-b");
}

TEST_F(CouchbaseReservationRepositoryIntegrationTest, QueriesCreatorAndPendingApprovalsByTenant) {
    const auto suffix = unique_suffix();
    const auto organization = haven::domain::OrganizationId{"organization-" + suffix};
    const auto creator = haven::domain::UserId{"creator-" + suffix};
    auto pending = make_reservation(organization,
                                    haven::domain::ReservationId{"pending-" + suffix},
                                    haven::domain::ResourceId{"resource-" + suffix},
                                    creator,
                                    haven::domain::TimeInterval{at(900), at(1'000)},
                                    "pending",
                                    haven::domain::ReservationStatus::PendingApproval);
    static_cast<void>(repository_->insert(organization, pending));

    const auto by_creator = repository_->find_by_creator(organization, creator);
    const auto approvals = repository_->find_pending_approvals(organization);
    ASSERT_EQ(by_creator.size(), 1U);
    ASSERT_EQ(approvals.size(), 1U);
    EXPECT_EQ(by_creator.front().reservation_id(), pending.reservation_id());
    EXPECT_EQ(approvals.front().reservation_id(), pending.reservation_id());
}

TEST_F(CouchbaseReservationRepositoryIntegrationTest,
       ConflictUsesConfirmedHalfOpenIntervalsAndSupportsExclusion) {
    const auto suffix = unique_suffix();
    const auto organization = haven::domain::OrganizationId{"organization-" + suffix};
    const auto resource = haven::domain::ResourceId{"resource-" + suffix};
    const auto confirmed_id = haven::domain::ReservationId{"confirmed-" + suffix};
    auto confirmed = make_reservation(organization,
                                      confirmed_id,
                                      resource,
                                      haven::domain::UserId{"creator-" + suffix},
                                      haven::domain::TimeInterval{at(1'100), at(1'200)},
                                      "confirmed");
    static_cast<void>(repository_->insert(organization, confirmed));
    auto pending = make_reservation(organization,
                                    haven::domain::ReservationId{"pending-" + suffix},
                                    resource,
                                    confirmed.created_by(),
                                    haven::domain::TimeInterval{at(1'300), at(1'400)},
                                    "pending",
                                    haven::domain::ReservationStatus::PendingApproval);
    static_cast<void>(repository_->insert(organization, pending));

    EXPECT_TRUE(repository_->has_conflict(
        organization, resource, haven::domain::TimeInterval{at(1'150), at(1'250)}));
    EXPECT_FALSE(repository_->has_conflict(
        organization, resource, haven::domain::TimeInterval{at(1'000), at(1'100)}));
    EXPECT_FALSE(repository_->has_conflict(
        organization, resource, haven::domain::TimeInterval{at(1'200), at(1'300)}));
    EXPECT_FALSE(repository_->has_conflict(
        organization, resource, haven::domain::TimeInterval{at(1'325), at(1'350)}));
    EXPECT_FALSE(repository_->has_conflict_excluding(
        organization, resource, haven::domain::TimeInterval{at(1'150), at(1'250)}, confirmed_id));
}

TEST_F(CouchbaseReservationRepositoryIntegrationTest,
       CalendarReturnsAllOverlappingStatesButNotAdjacentOrOtherResources) {
    const auto suffix = unique_suffix();
    const auto organization = haven::domain::OrganizationId{"organization-" + suffix};
    const auto resource = haven::domain::ResourceId{"resource-" + suffix};
    const auto creator = haven::domain::UserId{"creator-" + suffix};
    auto overlapping = make_reservation(organization,
                                        haven::domain::ReservationId{"overlap-" + suffix},
                                        resource,
                                        creator,
                                        haven::domain::TimeInterval{at(1'500), at(1'700)},
                                        "overlap",
                                        haven::domain::ReservationStatus::Cancelled);
    static_cast<void>(repository_->insert(organization, overlapping));
    auto adjacent = make_reservation(organization,
                                     haven::domain::ReservationId{"adjacent-" + suffix},
                                     resource,
                                     creator,
                                     haven::domain::TimeInterval{at(1'700), at(1'800)},
                                     "adjacent");
    static_cast<void>(repository_->insert(organization, adjacent));
    auto other_resource = make_reservation(organization,
                                           haven::domain::ReservationId{"other-" + suffix},
                                           haven::domain::ResourceId{"other-resource-" + suffix},
                                           creator,
                                           haven::domain::TimeInterval{at(1'550), at(1'650)},
                                           "other");
    static_cast<void>(repository_->insert(organization, other_resource));

    const auto results = repository_->find_by_resource_and_interval(
        organization, resource, haven::domain::TimeInterval{at(1'600), at(1'700)});
    ASSERT_EQ(results.size(), 1U);
    EXPECT_EQ(results.front().reservation_id(), overlapping.reservation_id());
    EXPECT_EQ(results.front().status(), haven::domain::ReservationStatus::Cancelled);
}

}  // namespace

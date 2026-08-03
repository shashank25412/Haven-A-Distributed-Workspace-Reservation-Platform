/**
 * @file couchbase_resource_repository_test.cpp
 * @brief Exercises the resource repository against a live Couchbase service.
 */

#include "haven/infrastructure/persistence/couchbase/couchbase_resource_repository.hpp"

#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/resource_status.hpp"
#include "haven/domain/value_objects/resource_type.hpp"
#include "haven/infrastructure/observability/metrics/no_op_metrics_recorder.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_collections.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_configuration.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_connection.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_document_key.hpp"
#include "haven/infrastructure/persistence/couchbase/resource_document.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <couchbase/codec/tao_json_serializer.hxx>
#include <couchbase/error_codes.hxx>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

namespace couchbase_persistence = haven::infrastructure::persistence::couchbase;

[[nodiscard]] std::optional<std::string> environment_value(const char* name) {
    const auto* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return std::nullopt;
    }
    return std::string{value};
}

[[nodiscard]] std::optional<couchbase_persistence::CouchbaseConfiguration>
integration_configuration() {
    const auto connection_string = environment_value("HVN_COUCHBASE_CONNECTION_STRING");
    const auto username = environment_value("HVN_COUCHBASE_USERNAME");
    const auto password = environment_value("HVN_COUCHBASE_PASSWORD");
    const auto bucket = environment_value("HVN_COUCHBASE_BUCKET");
    const auto scope = environment_value("HVN_COUCHBASE_SCOPE");

    if (!connection_string || !username || !password || !bucket || !scope) {
        return std::nullopt;
    }

    return couchbase_persistence::CouchbaseConfiguration{
        .connection_string = *connection_string,
        .username = *username,
        .password = *password,
        .bucket_name = *bucket,
        .scope_name = *scope,
    };
}

[[nodiscard]] std::string unique_suffix() {
    static std::uint64_t sequence{0};
    const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    ++sequence;
    return std::to_string(timestamp) + "-" + std::to_string(sequence);
}

class CouchbaseResourceRepositoryIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto configuration = integration_configuration();
        if (!configuration) {
            GTEST_SKIP() << "Set all HVN_COUCHBASE_* variables to run Couchbase integration tests";
        }

        connection_ = std::make_shared<couchbase_persistence::CouchbaseConnection>(*configuration);
        repository_ = std::make_unique<couchbase_persistence::CouchbaseResourceRepository>(
            connection_, metrics_);
    }

    void TearDown() override {
        if (!connection_) {
            return;
        }

        auto collection =
            connection_->collection(couchbase_persistence::CouchbaseCollections::resources);
        for (const auto& key : document_keys_) {
            auto [error, result] = collection.remove(key).get();
            static_cast<void>(result);
            if (error && error.ec() != ::couchbase::errc::key_value::document_not_found) {
                ADD_FAILURE() << "Failed to clean up Couchbase document " << key << ": "
                              << error.ec().message();
            }
        }
    }

    void store(const haven::domain::OrganizationId& organization_id,
               const haven::domain::ResourceId& resource_id,
               std::string name,
               std::string description,
               haven::domain::ResourceType type,
               haven::domain::ResourceStatus status,
               bool requires_approval,
               std::uint64_t version) {
        const auto key = couchbase_persistence::resource_document_key(organization_id, resource_id);
        const auto document = couchbase_persistence::ResourceDocument{
            .schema_version = couchbase_persistence::kResourceDocumentSchemaVersion,
            .resource_id = resource_id.value(),
            .organization_id = organization_id.value(),
            .name = std::move(name),
            .description = std::move(description),
            .resource_type = std::string{haven::domain::to_string(type)},
            .status = std::string{haven::domain::to_string(status)},
            .requires_approval = requires_approval,
            .version = version,
        };

        auto collection =
            connection_->collection(couchbase_persistence::CouchbaseCollections::resources);
        auto [error, result] =
            collection.upsert(key, couchbase_persistence::resource_document_to_json(document))
                .get();
        static_cast<void>(result);
        ASSERT_FALSE(error) << error.ec().message();
        document_keys_.push_back(key);
    }

    std::shared_ptr<couchbase_persistence::CouchbaseConnection> connection_;
    haven::infrastructure::observability::metrics::NoOpMetricsRecorder metrics_;
    std::unique_ptr<couchbase_persistence::CouchbaseResourceRepository> repository_;
    std::vector<std::string> document_keys_;
};

TEST_F(CouchbaseResourceRepositoryIntegrationTest, FindsTenantScopedResourceAndPreservesAllFields) {
    const auto suffix = unique_suffix();
    const auto organization_id = haven::domain::OrganizationId{"organization-" + suffix};
    const auto resource_id = haven::domain::ResourceId{"resource-" + suffix};
    store(organization_id,
          resource_id,
          "Architecture Room",
          "Quiet room",
          haven::domain::ResourceType::MeetingRoom,
          haven::domain::ResourceStatus::Inactive,
          true,
          7);

    const auto found = repository_->find_by_id(organization_id, resource_id);

    ASSERT_TRUE(found);
    const auto& resource = found->aggregate();
    EXPECT_EQ(resource.organization_id(), organization_id);
    EXPECT_EQ(resource.resource_id(), resource_id);
    EXPECT_EQ(resource.name(), "Architecture Room");
    EXPECT_EQ(resource.description(), "Quiet room");
    EXPECT_EQ(resource.type(), haven::domain::ResourceType::MeetingRoom);
    EXPECT_EQ(resource.status(), haven::domain::ResourceStatus::Inactive);
    EXPECT_TRUE(resource.requires_approval());
    EXPECT_EQ(resource.version().value(), 7U);
}

TEST_F(CouchbaseResourceRepositoryIntegrationTest, ReturnsEmptyForMissingAndWrongOrganization) {
    const auto suffix = unique_suffix();
    const auto owner = haven::domain::OrganizationId{"owner-" + suffix};
    const auto other = haven::domain::OrganizationId{"other-" + suffix};
    const auto resource_id = haven::domain::ResourceId{"resource-" + suffix};
    store(owner,
          resource_id,
          "Desk",
          "",
          haven::domain::ResourceType::OfficeDesk,
          haven::domain::ResourceStatus::Active,
          false,
          1);

    EXPECT_FALSE(repository_->find_by_id(other, resource_id));
    EXPECT_FALSE(repository_->find_by_id(owner, haven::domain::ResourceId{"missing-" + suffix}));
}

TEST_F(CouchbaseResourceRepositoryIntegrationTest, SameResourceIdIsIsolatedAcrossOrganizations) {
    const auto suffix = unique_suffix();
    const auto first_organization = haven::domain::OrganizationId{"first-" + suffix};
    const auto second_organization = haven::domain::OrganizationId{"second-" + suffix};
    const auto resource_id = haven::domain::ResourceId{"shared-" + suffix};
    store(first_organization,
          resource_id,
          "First tenant room",
          "",
          haven::domain::ResourceType::MeetingRoom,
          haven::domain::ResourceStatus::Active,
          false,
          1);
    store(second_organization,
          resource_id,
          "Second tenant room",
          "",
          haven::domain::ResourceType::MeetingRoom,
          haven::domain::ResourceStatus::Active,
          true,
          2);

    const auto first = repository_->find_by_id(first_organization, resource_id);
    const auto second = repository_->find_by_id(second_organization, resource_id);

    ASSERT_TRUE(first);
    ASSERT_TRUE(second);
    EXPECT_EQ(first->aggregate().name(), "First tenant room");
    EXPECT_EQ(second->aggregate().name(), "Second tenant room");
    EXPECT_NE(first->aggregate().requires_approval(), second->aggregate().requires_approval());
}

TEST_F(CouchbaseResourceRepositoryIntegrationTest,
       PointLoadsExposeStableTokenAndPreserveDomainVersion) {
    const auto suffix = unique_suffix();
    const auto organization_id = haven::domain::OrganizationId{"organization-" + suffix};
    const auto resource_id = haven::domain::ResourceId{"resource-" + suffix};
    store(organization_id,
          resource_id,
          "Stable room",
          "",
          haven::domain::ResourceType::MeetingRoom,
          haven::domain::ResourceStatus::Active,
          false,
          42);

    const auto first = repository_->find_by_id(organization_id, resource_id);
    const auto second = repository_->find_by_id(organization_id, resource_id);

    ASSERT_TRUE(first);
    ASSERT_TRUE(second);
    EXPECT_EQ(first->persistence_token(), second->persistence_token());
    EXPECT_EQ(first->aggregate().version().value(), 42U);
    EXPECT_EQ(second->aggregate().version().value(), 42U);
}

TEST_F(CouchbaseResourceRepositoryIntegrationTest,
       ExternalReplacementChangesTokenWithoutChangingDomainVersion) {
    const auto suffix = unique_suffix();
    const auto organization_id = haven::domain::OrganizationId{"organization-" + suffix};
    const auto resource_id = haven::domain::ResourceId{"resource-" + suffix};
    store(organization_id,
          resource_id,
          "Replaceable room",
          "",
          haven::domain::ResourceType::MeetingRoom,
          haven::domain::ResourceStatus::Active,
          false,
          9);
    const auto before = repository_->find_by_id(organization_id, resource_id);
    ASSERT_TRUE(before);

    const auto key = couchbase_persistence::resource_document_key(organization_id, resource_id);
    const auto replacement = couchbase_persistence::ResourceDocument{
        .schema_version = couchbase_persistence::kResourceDocumentSchemaVersion,
        .resource_id = resource_id.value(),
        .organization_id = organization_id.value(),
        .name = "Replaced room",
        .description = "",
        .resource_type =
            std::string{haven::domain::to_string(haven::domain::ResourceType::MeetingRoom)},
        .status = std::string{haven::domain::to_string(haven::domain::ResourceStatus::Active)},
        .requires_approval = false,
        .version = 9,
    };
    auto collection =
        connection_->collection(couchbase_persistence::CouchbaseCollections::resources);
    auto [error, result] =
        collection.replace(key, couchbase_persistence::resource_document_to_json(replacement))
            .get();
    static_cast<void>(result);
    ASSERT_FALSE(error) << error.ec().message();

    const auto after = repository_->find_by_id(organization_id, resource_id);

    ASSERT_TRUE(after);
    EXPECT_NE(before->persistence_token(), after->persistence_token());
    EXPECT_EQ(after->aggregate().name(), "Replaced room");
    EXPECT_EQ(after->aggregate().version().value(), 9U);
}

TEST_F(CouchbaseResourceRepositoryIntegrationTest,
       FindsOnlyActiveResourcesOfRequestedTypeWithinOrganization) {
    const auto suffix = unique_suffix();
    const auto organization_id = haven::domain::OrganizationId{"organization-" + suffix};
    const auto active_room = haven::domain::ResourceId{"active-room-" + suffix};
    store(organization_id,
          active_room,
          "Active room",
          "",
          haven::domain::ResourceType::MeetingRoom,
          haven::domain::ResourceStatus::Active,
          false,
          1);
    store(organization_id,
          haven::domain::ResourceId{"inactive-room-" + suffix},
          "Inactive room",
          "",
          haven::domain::ResourceType::MeetingRoom,
          haven::domain::ResourceStatus::Inactive,
          false,
          1);
    store(organization_id,
          haven::domain::ResourceId{"active-desk-" + suffix},
          "Active desk",
          "",
          haven::domain::ResourceType::OfficeDesk,
          haven::domain::ResourceStatus::Active,
          false,
          1);

    const auto resources =
        repository_->find_active_by_type(organization_id, haven::domain::ResourceType::MeetingRoom);

    ASSERT_EQ(resources.size(), 1U);
    EXPECT_EQ(resources.front().resource_id(), active_room);
}

}  // namespace

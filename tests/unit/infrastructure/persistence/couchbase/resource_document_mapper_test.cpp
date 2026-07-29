/**
 * @file resource_document_mapper_test.cpp
 * @brief Tests mapping between resources and Couchbase documents.
 */

#include "haven/infrastructure/persistence/couchbase/resource_document_mapper.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

namespace haven::infrastructure::persistence::couchbase {
namespace {

[[nodiscard]] domain::Resource resource(
    const domain::ResourceStatus status = domain::ResourceStatus::Active,
    const bool requires_approval = true,
    const domain::ResourceType type = domain::ResourceType::MeetingRoom,
    std::string description = "Eight-seat room") {
    return domain::Resource::rehydrate(
        domain::OrganizationId{"organization-123"},
        domain::ResourceId{"resource-123"},
        "Atlas Meeting Room",
        std::move(description),
        type,
        status,
        requires_approval,
        domain::Version{42});
}

[[nodiscard]] ResourceDocument document(
    const std::string& status = "ACTIVE") {
    return ResourceDocument{
        .schema_version = kResourceDocumentSchemaVersion,
        .resource_id = "resource-123",
        .organization_id = "organization-123",
        .name = "Atlas Meeting Room",
        .description = "Eight-seat room",
        .resource_type = "MEETING_ROOM",
        .status = status,
        .requires_approval = true,
        .version = 42,
    };
}

TEST(ResourceDocumentMapperTest, MapsActiveResourceToDocument) {
    const ResourceDocument mapped = to_resource_document(resource());

    EXPECT_EQ(mapped.schema_version, kResourceDocumentSchemaVersion);
    EXPECT_EQ(mapped.resource_id, "resource-123");
    EXPECT_EQ(mapped.organization_id, "organization-123");
    EXPECT_EQ(mapped.name, "Atlas Meeting Room");
    EXPECT_EQ(mapped.status, "ACTIVE");
}

TEST(ResourceDocumentMapperTest, MapsInactiveResourceToDocument) {
    const ResourceDocument mapped =
        to_resource_document(resource(domain::ResourceStatus::Inactive));

    EXPECT_EQ(mapped.status, "INACTIVE");
}

TEST(ResourceDocumentMapperTest, PreservesApprovalRequirement) {
    const ResourceDocument mapped = to_resource_document(
        resource(domain::ResourceStatus::Active, false));

    EXPECT_FALSE(mapped.requires_approval);
}

TEST(ResourceDocumentMapperTest, PreservesResourceType) {
    const ResourceDocument mapped = to_resource_document(resource(
        domain::ResourceStatus::Active,
        true,
        domain::ResourceType::ParkingSlot));

    EXPECT_EQ(mapped.resource_type, "PARKING_SLOT");
}

TEST(ResourceDocumentMapperTest, PreservesEmptyDescription) {
    const ResourceDocument mapped = to_resource_document(resource(
        domain::ResourceStatus::Active,
        true,
        domain::ResourceType::MeetingRoom,
        ""));

    EXPECT_TRUE(mapped.description.empty());
}

TEST(ResourceDocumentMapperTest, PreservesPersistenceVersion) {
    const ResourceDocument mapped = to_resource_document(resource());

    EXPECT_EQ(mapped.version, 42);
}

TEST(ResourceDocumentMapperTest, RehydratesActiveResourceDocument) {
    const domain::Resource mapped = to_domain_resource(document());

    EXPECT_TRUE(mapped.is_active());
    EXPECT_EQ(mapped.status(), domain::ResourceStatus::Active);
}

TEST(ResourceDocumentMapperTest, RehydratesInactiveResourceDocument) {
    const domain::Resource mapped = to_domain_resource(document("INACTIVE"));

    EXPECT_FALSE(mapped.is_active());
    EXPECT_EQ(mapped.status(), domain::ResourceStatus::Inactive);
}

TEST(ResourceDocumentMapperTest, RoundTripPreservesAllPersistedState) {
    const domain::Resource original = resource(
        domain::ResourceStatus::Inactive,
        false,
        domain::ResourceType::HotelRoom,
        "");

    const domain::Resource mapped =
        to_domain_resource(to_resource_document(original));

    EXPECT_EQ(mapped.resource_id(), original.resource_id());
    EXPECT_EQ(mapped.organization_id(), original.organization_id());
    EXPECT_EQ(mapped.name(), original.name());
    EXPECT_EQ(mapped.description(), original.description());
    EXPECT_EQ(mapped.type(), original.type());
    EXPECT_EQ(mapped.status(), original.status());
    EXPECT_EQ(mapped.requires_approval(), original.requires_approval());
    EXPECT_EQ(mapped.version(), original.version());
}

TEST(ResourceDocumentMapperTest, RejectsUnknownResourceType) {
    auto persisted = document();
    persisted.resource_type = "ROOFTOP";

    EXPECT_THROW(
        static_cast<void>(to_domain_resource(persisted)),
        std::invalid_argument);
}

TEST(ResourceDocumentMapperTest, RejectsUnknownResourceStatus) {
    auto persisted = document();
    persisted.status = "ARCHIVED";

    EXPECT_THROW(
        static_cast<void>(to_domain_resource(persisted)),
        std::invalid_argument);
}

TEST(ResourceDocumentMapperTest, RejectsUnsupportedSchemaVersion) {
    auto persisted = document();
    persisted.schema_version = kResourceDocumentSchemaVersion + 1;

    EXPECT_THROW(
        static_cast<void>(to_domain_resource(persisted)),
        std::invalid_argument);
}

}  // namespace
}  // namespace haven::infrastructure::persistence::couchbase

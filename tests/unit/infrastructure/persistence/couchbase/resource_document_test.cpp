/**
 * @file resource_document_test.cpp
 * @brief Tests Couchbase resource-document JSON conversion.
 */

#include "haven/infrastructure/persistence/couchbase/resource_document.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

namespace haven::infrastructure::persistence::couchbase {
namespace {

[[nodiscard]] ResourceDocument create_resource_document() {
    return ResourceDocument{
        .schema_version = kResourceDocumentSchemaVersion,
        .resource_id = "res_01J2H6M5J5W7P3C7Q9R8T6V4B2",
        .organization_id = "org_01J2H6M5J5W7P3C7Q9R8T6V4B2",
        .name = "Atlas Meeting Room",
        .description = "Eight-seat meeting room with video conferencing",
        .resource_type = "MEETING_ROOM",
        .status = "ACTIVE",
        .requires_approval = true,
        .version = 1,
    };
}

TEST(ResourceDocumentTest, SerializesResourceDocumentToJson) {
    const auto document = create_resource_document();

    const auto json = resource_document_to_json(document);

    EXPECT_EQ(json.at("documentType").get_string(), kResourceDocumentType);
    EXPECT_EQ(json.at("schemaVersion").get_unsigned(), document.schema_version);
    EXPECT_EQ(json.at("resourceId").get_string(), document.resource_id);
    EXPECT_EQ(json.at("organizationId").get_string(), document.organization_id);
    EXPECT_EQ(json.at("name").get_string(), document.name);
    EXPECT_EQ(json.at("description").get_string(), document.description);
    EXPECT_EQ(json.at("resourceType").get_string(), document.resource_type);
    EXPECT_EQ(json.at("status").get_string(), document.status);
    EXPECT_EQ(json.at("requiresApproval").get_boolean(), document.requires_approval);
    EXPECT_EQ(json.at("version").get_unsigned(), document.version);
}

TEST(ResourceDocumentTest, DeserializesResourceDocumentFromJson) {
    const auto expected = create_resource_document();

    const auto actual = resource_document_from_json(
        resource_document_to_json(expected));

    EXPECT_EQ(actual.schema_version, expected.schema_version);
    EXPECT_EQ(actual.resource_id, expected.resource_id);
    EXPECT_EQ(actual.organization_id, expected.organization_id);
    EXPECT_EQ(actual.name, expected.name);
    EXPECT_EQ(actual.description, expected.description);
    EXPECT_EQ(actual.resource_type, expected.resource_type);
    EXPECT_EQ(actual.status, expected.status);
    EXPECT_EQ(actual.requires_approval, expected.requires_approval);
    EXPECT_EQ(actual.version, expected.version);
}

TEST(ResourceDocumentTest, RejectsIncorrectDocumentType) {
    auto json = resource_document_to_json(create_resource_document());
    json["documentType"] = "reservation";

    EXPECT_THROW(
        static_cast<void>(resource_document_from_json(json)),
        std::invalid_argument
    );
}

TEST(ResourceDocumentTest, RejectsMissingDocumentType) {
    auto json = resource_document_to_json(create_resource_document());
    json.erase("documentType");

    EXPECT_THROW(static_cast<void>(resource_document_from_json(json)), std::exception);
}

TEST(ResourceDocumentTest, RejectsMissingSchemaVersion) {
    auto json = resource_document_to_json(create_resource_document());
    json.erase("schemaVersion");

    EXPECT_THROW(static_cast<void>(resource_document_from_json(json)), std::exception);
}

TEST(ResourceDocumentTest, RejectsUnsupportedSchemaVersion) {
    auto json = resource_document_to_json(create_resource_document());
    json["schemaVersion"] = kResourceDocumentSchemaVersion + 1;

    EXPECT_THROW(
        static_cast<void>(resource_document_from_json(json)),
        std::invalid_argument
    );
}

TEST(ResourceDocumentTest, RejectsMissingRequiredField) {
    auto json = resource_document_to_json(create_resource_document());
    json.erase("resourceId");

    EXPECT_THROW(static_cast<void>(resource_document_from_json(json)), std::exception);
}

TEST(ResourceDocumentTest, RejectsIncorrectFieldType) {
    auto json = resource_document_to_json(create_resource_document());
    json["requiresApproval"] = "true";

    EXPECT_THROW(static_cast<void>(resource_document_from_json(json)), std::exception);
}

TEST(ResourceDocumentTest, IgnoresUnknownAdditionalFields) {
    auto json = resource_document_to_json(create_resource_document());
    json["futureField"] = "future-value";

    EXPECT_NO_THROW(static_cast<void>(resource_document_from_json(json)));
}

TEST(ResourceDocumentTest, RejectsInvalidDocumentDuringSerialization) {
    auto document = create_resource_document();
    document.resource_id.clear();

    EXPECT_THROW(
        static_cast<void>(resource_document_to_json(document)),
        std::invalid_argument
    );
}

}  // namespace
}  // namespace haven::infrastructure::persistence::couchbase

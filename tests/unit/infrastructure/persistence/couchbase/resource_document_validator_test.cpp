/**
 * @file resource_document_validator_test.cpp
 * @brief Tests validation of persisted Couchbase resource documents.
 */

#include "haven/infrastructure/persistence/couchbase/resource_document_validator.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

namespace haven::infrastructure::persistence::couchbase {
namespace {

[[nodiscard]] ResourceDocument valid_resource_document() {
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

TEST(ResourceDocumentValidatorTest, AcceptsValidResourceDocument) {
    EXPECT_NO_THROW(validate_resource_document(valid_resource_document()));
}

TEST(ResourceDocumentValidatorTest, AcceptsEmptyDescription) {
    auto document = valid_resource_document();
    document.description.clear();

    EXPECT_NO_THROW(validate_resource_document(document));
}

TEST(ResourceDocumentValidatorTest, RejectsUnsupportedSchemaVersion) {
    auto document = valid_resource_document();
    document.schema_version = kResourceDocumentSchemaVersion + 1;

    EXPECT_THROW(validate_resource_document(document), std::invalid_argument);
}

TEST(ResourceDocumentValidatorTest, RejectsEmptyResourceId) {
    auto document = valid_resource_document();
    document.resource_id.clear();

    EXPECT_THROW(validate_resource_document(document), std::invalid_argument);
}

TEST(ResourceDocumentValidatorTest, RejectsEmptyOrganizationId) {
    auto document = valid_resource_document();
    document.organization_id.clear();

    EXPECT_THROW(validate_resource_document(document), std::invalid_argument);
}

TEST(ResourceDocumentValidatorTest, RejectsEmptyName) {
    auto document = valid_resource_document();
    document.name.clear();

    EXPECT_THROW(validate_resource_document(document), std::invalid_argument);
}

TEST(ResourceDocumentValidatorTest, RejectsEmptyResourceType) {
    auto document = valid_resource_document();
    document.resource_type.clear();

    EXPECT_THROW(validate_resource_document(document), std::invalid_argument);
}

TEST(ResourceDocumentValidatorTest, RejectsEmptyStatus) {
    auto document = valid_resource_document();
    document.status.clear();

    EXPECT_THROW(validate_resource_document(document), std::invalid_argument);
}

TEST(ResourceDocumentValidatorTest, RejectsZeroVersion) {
    auto document = valid_resource_document();
    document.version = 0;

    EXPECT_THROW(validate_resource_document(document), std::invalid_argument);
}

}  // namespace
}  // namespace haven::infrastructure::persistence::couchbase
/**
 * @file couchbase_document_key_test.cpp
 * @brief Tests tenant-scoped Couchbase document-key generation.
 */

#include "haven/infrastructure/persistence/couchbase/couchbase_document_key.hpp"

#include <gtest/gtest.h>

namespace haven::infrastructure::persistence::couchbase {
namespace {

TEST(CouchbaseDocumentKeyTest, CreatesTenantScopedResourceDocumentKey) {
    const haven::domain::OrganizationId organization_id{
        "01950b31-e82a-7db7-8f69-820edc75a440"};

    const haven::domain::ResourceId resource_id{
        "01950b31-e82a-7db7-8f69-820edc75a441"};

    EXPECT_EQ(
        resource_document_key(organization_id, resource_id),
        "resource::01950b31-e82a-7db7-8f69-820edc75a440::"
        "01950b31-e82a-7db7-8f69-820edc75a441");
}

TEST(CouchbaseDocumentKeyTest, CreatesTenantScopedReservationDocumentKey) {
    const haven::domain::OrganizationId organization_id{
        "01950b31-e82a-7db7-8f69-820edc75a440"};

    const haven::domain::ReservationId reservation_id{
        "01950b31-e82a-7db7-8f69-820edc75a442"};

    EXPECT_EQ(
        reservation_document_key(organization_id, reservation_id),
        "reservation::01950b31-e82a-7db7-8f69-820edc75a440::"
        "01950b31-e82a-7db7-8f69-820edc75a442");
}

TEST(CouchbaseDocumentKeyTest, ProducesDifferentKeysForDifferentOrganizations) {
    const haven::domain::OrganizationId first_organization_id{
        "01950b31-e82a-7db7-8f69-820edc75a440"};

    const haven::domain::OrganizationId second_organization_id{
        "01950b31-e82a-7db7-8f69-820edc75a441"};

    const haven::domain::ResourceId resource_id{
        "01950b31-e82a-7db7-8f69-820edc75a442"};

    EXPECT_NE(
        resource_document_key(first_organization_id, resource_id),
        resource_document_key(second_organization_id, resource_id));
}

}  // namespace
}  // namespace haven::infrastructure::persistence::couchbase
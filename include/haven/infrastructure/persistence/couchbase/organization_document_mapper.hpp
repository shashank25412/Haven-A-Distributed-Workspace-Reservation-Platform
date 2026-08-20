/**
 * @file organization_document_mapper.hpp
 * @brief Declares mapping between organizations and Couchbase documents.
 */

#pragma once

#include "haven/domain/organization.hpp"
#include "haven/infrastructure/persistence/couchbase/organization_document.hpp"

namespace haven::infrastructure::persistence::couchbase {

/** @brief Maps a domain organization to its Couchbase persistence representation. */
[[nodiscard]] OrganizationDocument to_organization_document(const domain::Organization& organization);

/** @brief Rehydrates a domain organization from a Couchbase persistence document. */
[[nodiscard]] domain::Organization to_domain_organization(const OrganizationDocument& document);

}  // namespace haven::infrastructure::persistence::couchbase

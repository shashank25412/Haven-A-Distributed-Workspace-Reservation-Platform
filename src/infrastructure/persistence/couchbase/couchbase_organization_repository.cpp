/**
 * @file couchbase_organization_repository.cpp
 * @brief Implements the Couchbase-backed organization directory repository adapter.
 */

#include "haven/infrastructure/persistence/couchbase/couchbase_organization_repository.hpp"

#include "haven/application/repository_error.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_collections.hpp"
#include "haven/infrastructure/persistence/couchbase/organization_document.hpp"
#include "haven/infrastructure/persistence/couchbase/organization_document_mapper.hpp"
#include "haven/logging/logging.hpp"

#include <couchbase/codec/tao_json_serializer.hxx>
#include <couchbase/error.hxx>
#include <couchbase/error_codes.hxx>
#include <couchbase/query_options.hxx>
#include <couchbase/query_scan_consistency.hxx>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace haven::infrastructure::persistence::couchbase {

namespace {

using haven::application::RepositoryError;
using haven::application::RepositoryErrorCode;

[[nodiscard]] RepositoryError translate_error(const ::couchbase::error& error,
                                              const std::string_view operation) {
    const auto error_code = error.ec();

    auto code = RepositoryErrorCode::Persistence;
    if (error_code == ::couchbase::errc::common::authentication_failure) {
        code = RepositoryErrorCode::Authentication;
    } else if (error_code == ::couchbase::errc::key_value::xattr_no_access) {
        code = RepositoryErrorCode::Authorization;
    } else if (error_code == ::couchbase::errc::common::ambiguous_timeout ||
               error_code == ::couchbase::errc::common::unambiguous_timeout) {
        code = RepositoryErrorCode::Timeout;
    }

    return RepositoryError{code, std::string{operation} + " failed: " + error_code.message()};
}

[[nodiscard]] std::string all_organizations_query() {
    return "SELECT organization.* FROM `" + std::string{CouchbaseCollections::organizations} +
           "` AS organization WHERE organization.documentType = \"organization\"";
}

}  // namespace

CouchbaseOrganizationRepository::CouchbaseOrganizationRepository(
    std::shared_ptr<CouchbaseConnection> connection)
    : connection_(std::move(connection)) {
    if (!connection_) {
        throw std::invalid_argument("Couchbase organization repository connection must not be null");
    }
}

std::vector<haven::domain::Organization> CouchbaseOrganizationRepository::find_all() const {
    HVN_TRACE_SCOPE();

    auto options = ::couchbase::query_options{};
    options.readonly(true).scan_consistency(::couchbase::query_scan_consistency::request_plus);

    auto [error, result] = connection_->scope().query(all_organizations_query(), options).get();
    if (error) {
        HVN_ERROR_LOG("Couchbase organization listing failed: ", error.ec().message());
        throw translate_error(error, "Couchbase organization listing");
    }

    auto organizations = std::vector<haven::domain::Organization>{};
    const auto rows = result.rows_as();
    organizations.reserve(rows.size());
    try {
        for (const auto& row : rows) {
            organizations.push_back(to_domain_organization(organization_document_from_json(row)));
        }
    } catch (const std::exception& exception) {
        HVN_ERROR_LOG("Couchbase organization listing returned an invalid document: ",
                      exception.what());
        throw RepositoryError{RepositoryErrorCode::Persistence,
                              "Couchbase organization listing returned an invalid document"};
    }

    std::sort(organizations.begin(), organizations.end(), [](const auto& left, const auto& right) {
        return left.name() < right.name();
    });

    return organizations;
}

}  // namespace haven::infrastructure::persistence::couchbase

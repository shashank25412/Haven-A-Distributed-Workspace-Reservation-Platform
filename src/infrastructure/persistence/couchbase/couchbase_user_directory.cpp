/**
 * @file couchbase_user_directory.cpp
 * @brief Implements the Couchbase-backed user directory adapter.
 */

#include "haven/infrastructure/persistence/couchbase/couchbase_user_directory.hpp"

#include "haven/infrastructure/persistence/couchbase/couchbase_collections.hpp"
#include "haven/logging/logging.hpp"

#include <couchbase/codec/tao_json_serializer.hxx>
#include <couchbase/query_options.hxx>
#include <couchbase/query_scan_consistency.hxx>

#include <stdexcept>
#include <utility>

namespace haven::infrastructure::persistence::couchbase {
namespace {

[[nodiscard]] std::string find_by_user_id_query() {
    return "SELECT credential.name FROM `" + std::string{CouchbaseCollections::credentials} +
           "` AS credential WHERE credential.documentType = \"userCredential\" "
           "AND credential.userId = $userId LIMIT 1";
}

}  // namespace

CouchbaseUserDirectory::CouchbaseUserDirectory(std::shared_ptr<CouchbaseConnection> connection)
    : connection_(std::move(connection)) {
    if (!connection_) {
        throw std::invalid_argument("Couchbase user directory connection must not be null");
    }
}

std::optional<std::string> CouchbaseUserDirectory::find_display_name(
    const std::string_view user_id) const {
    HVN_TRACE_SCOPE();

    auto options = ::couchbase::query_options{};
    options.readonly(true).scan_consistency(::couchbase::query_scan_consistency::request_plus);
    options.named_parameters(std::make_pair("userId", std::string{user_id}));

    auto [error, result] = connection_->scope().query(find_by_user_id_query(), options).get();
    if (error) {
        HVN_ERROR_LOG("Couchbase user directory lookup failed: ", error.ec().message());
        return std::nullopt;
    }

    const auto rows = result.rows_as();
    if (rows.empty()) {
        return std::nullopt;
    }

    try {
        const auto& object = rows.front().get_object();
        const auto found = object.find("name");
        if (found == object.end() || found->second.get_string().empty()) {
            return std::nullopt;
        }
        return found->second.get_string();
    } catch (const std::exception& exception) {
        HVN_ERROR_LOG("Couchbase user directory lookup returned an invalid document: ",
                      exception.what());
        return std::nullopt;
    }
}

}  // namespace haven::infrastructure::persistence::couchbase

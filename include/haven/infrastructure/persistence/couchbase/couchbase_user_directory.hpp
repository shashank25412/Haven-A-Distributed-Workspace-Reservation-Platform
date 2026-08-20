/**
 * @file couchbase_user_directory.hpp
 * @brief Declares the Couchbase-backed user directory adapter.
 */

#pragma once

#include "haven/application/users/user_directory.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_connection.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace haven::infrastructure::persistence::couchbase {

/**
 * @brief Resolves display names by querying the identity credentials collection.
 */
class CouchbaseUserDirectory final : public haven::application::users::UserDirectory {
public:
    /**
     * @brief Constructs a directory using the identity Couchbase connection.
     *
     * @throws std::invalid_argument If connection is null.
     */
    explicit CouchbaseUserDirectory(std::shared_ptr<CouchbaseConnection> connection);

    [[nodiscard]] std::optional<std::string> find_display_name(
        std::string_view user_id) const override;

private:
    std::shared_ptr<CouchbaseConnection> connection_;
};

}  // namespace haven::infrastructure::persistence::couchbase

/**
 * @file couchbase_collections.hpp
 * @brief Defines Couchbase collection names used by Haven persistence.
 */

#pragma once

#include <string_view>

namespace haven::infrastructure::persistence::couchbase {

/**
 * @brief Contains the canonical Couchbase collection names used by Haven.
 */
struct CouchbaseCollections final {
    static constexpr std::string_view resources{"resources"};
    static constexpr std::string_view reservations{"reservations"};
    static constexpr std::string_view outbox{"outbox"};
    static constexpr std::string_view idempotency{"idempotency"};
    static constexpr std::string_view organizations{"organizations"};
};

}  // namespace haven::infrastructure::persistence::couchbase

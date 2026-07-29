/**
 * @file couchbase_cas.hpp
 * @brief Converts between Couchbase CAS values and opaque application tokens.
 */

#pragma once

#include "haven/application/persistence/persistence_token.hpp"

#include <couchbase/cas.hxx>

namespace haven::infrastructure::persistence::couchbase {

[[nodiscard]] inline haven::application::persistence::PersistenceToken persistence_token_from(
    const ::couchbase::cas cas) {
    return haven::application::persistence::PersistenceToken{cas.value()};
}

[[nodiscard]] inline ::couchbase::cas couchbase_cas_from(
    const haven::application::persistence::PersistenceToken& token) {
    return ::couchbase::cas{
        haven::application::persistence::PersistenceTokenAccess::representation(token)};
}

}  // namespace haven::infrastructure::persistence::couchbase

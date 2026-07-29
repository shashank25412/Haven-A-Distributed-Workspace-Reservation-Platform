/**
 * @file persistence_token.hpp
 * @brief Defines an opaque application persistence token.
 */

#pragma once

#include <compare>
#include <cstdint>
#include <stdexcept>

namespace haven::application::persistence {

class PersistenceTokenAccess;

/**
 * @brief Identifies one persisted revision of an aggregate.
 *
 * The representation is compatible with datastore revision identifiers, but
 * the token is opaque: callers must not interpret it numerically. It has no
 * relationship to an aggregate's domain Version.
 */
class PersistenceToken final {
public:
    /**
     * @brief Constructs a non-empty opaque persistence token.
     *
     * @throws std::invalid_argument If representation is zero.
     */
    explicit PersistenceToken(const std::uint64_t representation)
        : representation_(representation) {
        if (representation == 0) {
            throw std::invalid_argument("Persistence token must not be empty.");
        }
    }

    bool operator==(const PersistenceToken&) const = default;

private:
    friend class PersistenceTokenAccess;
    std::uint64_t representation_;
};

/**
 * @brief Provides persistence adapters access to the token representation.
 *
 * Application orchestration must not use this adapter boundary or interpret
 * the returned value.
 */
class PersistenceTokenAccess final {
public:
    [[nodiscard]] static std::uint64_t representation(const PersistenceToken& token) noexcept {
        return token.representation_;
    }
};

}  // namespace haven::application::persistence

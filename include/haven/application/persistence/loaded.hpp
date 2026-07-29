/**
 * @file loaded.hpp
 * @brief Defines an aggregate loaded with its persistence token.
 */

#pragma once

#include "haven/application/persistence/persistence_token.hpp"

#include <utility>

namespace haven::application::persistence {

/**
 * @brief Owns an aggregate and the opaque token for its persisted revision.
 *
 * @tparam Aggregate Aggregate value owned by the wrapper.
 */
template <typename Aggregate>
class Loaded final {
public:
    Loaded(Aggregate aggregate, PersistenceToken persistence_token)
        : aggregate_(std::move(aggregate)), persistence_token_(std::move(persistence_token)) {}

    [[nodiscard]] const Aggregate& aggregate() const noexcept {
        return aggregate_;
    }

    [[nodiscard]] Aggregate& aggregate() noexcept {
        return aggregate_;
    }

    [[nodiscard]] const PersistenceToken& persistence_token() const noexcept {
        return persistence_token_;
    }

private:
    Aggregate aggregate_;
    PersistenceToken persistence_token_;
};

}  // namespace haven::application::persistence

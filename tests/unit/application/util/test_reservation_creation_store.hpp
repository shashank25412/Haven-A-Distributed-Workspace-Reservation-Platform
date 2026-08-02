/**
 * @file test_reservation_creation_store.hpp
 * @brief Defines a configurable atomic reservation-creation store for application tests.
 */

#pragma once

#include "haven/application/repository_error.hpp"
#include "haven/application/reservations/reservation_creation_store.hpp"

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace haven::tests::util::application {

/** @brief Records successful persistence units and supports deterministic failure injection. */
class TestReservationCreationStore final
    : public haven::application::reservations::ReservationCreationStore {
public:
    [[nodiscard]] haven::application::persistence::PersistenceToken persist(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::Reservation& reservation,
        std::vector<haven::domain::ReservationDomainEvent> domain_events) override {
        ++persist_call_count_;
        if (force_failure_) {
            throw haven::application::RepositoryError{
                haven::application::RepositoryErrorCode::Persistence,
                "Forced reservation creation persistence failure"};
        }

        persisted_organization_id_ = organization_id;
        persisted_reservation_ = reservation;
        persisted_domain_events_ = std::move(domain_events);
        return persistence_token_;
    }

    void set_persistence_token(haven::application::persistence::PersistenceToken token) noexcept {
        persistence_token_ = token;
    }

    void force_failure(const bool enabled = true) noexcept {
        force_failure_ = enabled;
    }

    [[nodiscard]] std::size_t persist_call_count() const noexcept {
        return persist_call_count_;
    }

    [[nodiscard]] const std::optional<haven::domain::OrganizationId>& persisted_organization_id()
        const noexcept {
        return persisted_organization_id_;
    }

    [[nodiscard]] const std::optional<haven::domain::Reservation>& persisted_reservation()
        const noexcept {
        return persisted_reservation_;
    }

    [[nodiscard]] const std::vector<haven::domain::ReservationDomainEvent>&
    persisted_domain_events() const noexcept {
        return persisted_domain_events_;
    }

private:
    haven::application::persistence::PersistenceToken persistence_token_{1};
    bool force_failure_{false};
    std::size_t persist_call_count_{};
    std::optional<haven::domain::OrganizationId> persisted_organization_id_;
    std::optional<haven::domain::Reservation> persisted_reservation_;
    std::vector<haven::domain::ReservationDomainEvent> persisted_domain_events_;
};

}  // namespace haven::tests::util::application

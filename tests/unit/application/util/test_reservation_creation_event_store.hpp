/**
 * @file test_reservation_creation_event_store.hpp
 * @brief Defines a configurable creation-event recovery store for tests.
 */
#pragma once

#include "haven/application/repository_error.hpp"
#include "haven/application/reservations/reservation_creation_event_store.hpp"

#include <atomic>
#include <cstddef>
#include <mutex>
#include <optional>
#include <vector>

namespace haven::tests::util::application {

class TestReservationCreationEventStore final
    : public haven::application::reservations::ReservationCreationEventStore {
public:
    [[nodiscard]] bool contains_all(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ReservationId& reservation_id,
        const std::vector<haven::domain::EventId>& event_ids) const override {
        ++call_count_;
        const auto lock = std::scoped_lock{mutex_};
        organization_id_ = organization_id;
        reservation_id_ = reservation_id;
        event_ids_ = event_ids;
        if (error_)
            throw *error_;
        return result_;
    }
    void set_result(bool result) noexcept {
        result_ = result;
    }
    void set_error(haven::application::RepositoryError error) {
        error_ = std::move(error);
    }
    [[nodiscard]] std::size_t call_count() const noexcept {
        return call_count_.load();
    }
    [[nodiscard]] std::optional<haven::domain::OrganizationId> organization_id() const {
        const auto lock = std::scoped_lock{mutex_};
        return organization_id_;
    }
    [[nodiscard]] std::optional<haven::domain::ReservationId> reservation_id() const {
        const auto lock = std::scoped_lock{mutex_};
        return reservation_id_;
    }
    [[nodiscard]] std::vector<haven::domain::EventId> event_ids() const {
        const auto lock = std::scoped_lock{mutex_};
        return event_ids_;
    }

private:
    bool result_{true};
    std::optional<haven::application::RepositoryError> error_;
    mutable std::atomic_size_t call_count_{};
    mutable std::mutex mutex_;
    mutable std::optional<haven::domain::OrganizationId> organization_id_;
    mutable std::optional<haven::domain::ReservationId> reservation_id_;
    mutable std::vector<haven::domain::EventId> event_ids_;
};

}  // namespace haven::tests::util::application

/**
 * @file test_reservation_repository.hpp
 * @brief Defines a configurable reservation repository for application tests.
 */

#pragma once

#include "haven/application/repository_error.hpp"
#include "haven/application/reservations/reservation_repository.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace haven::tests::util::application {

/**
 * @brief Provides configurable reservation repository behavior for application tests.
 *
 * Query results are empty and conflict checks are false by default. Every
 * operation records its supplied arguments, and save retains a copy of the
 * reservation.
 */
class TestReservationRepository final
    : public haven::application::reservations::ReservationRepository {
public:
    void set_lookup_result(haven::application::reservations::ReservationLookupResult result) {
        lookup_result_ = std::move(result);
    }

    void set_lookup_result(std::optional<haven::domain::Reservation> result) {
        if (!result.has_value()) {
            lookup_result_ = std::nullopt;
            return;
        }
        lookup_result_.emplace(std::move(*result),
                               haven::application::persistence::PersistenceToken{current_token_});
    }

    void set_creator_result(haven::application::reservations::ReservationListResult result) {
        creator_result_ = std::move(result);
    }

    void set_pending_approvals_result(
        haven::application::reservations::ReservationListResult result) {
        pending_approvals_result_ = std::move(result);
    }

    void set_decided_approvals_result(
        haven::application::reservations::ReservationListResult result) {
        decided_approvals_result_ = std::move(result);
    }

    void set_all_result(haven::application::reservations::ReservationListResult result) {
        all_result_ = std::move(result);
    }

    void set_calendar_result(haven::application::reservations::ReservationListResult result) {
        calendar_result_ = std::move(result);
    }

    void set_conflict(const bool result) noexcept {
        conflict_result_ = result;
        conflict_results_.clear();
    }

    void set_conflict_results(std::vector<bool> results) {
        conflict_results_ = std::move(results);
    }

    void set_conflict_excluding(const bool result) noexcept {
        conflict_excluding_result_ = result;
    }

    [[nodiscard]] haven::application::reservations::ReservationLookupResult find_by_id(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ReservationId& reservation_id) const override {
        find_by_id_called_ = true;
        lookup_organization_id_ = organization_id;
        lookup_reservation_id_ = reservation_id;
        return lookup_result_;
    }

    [[nodiscard]] haven::application::reservations::ReservationListResult find_by_creator(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::UserId& caller_id) const override {
        find_by_creator_called_ = true;
        creator_organization_id_ = organization_id;
        creator_caller_id_ = caller_id;
        return creator_result_;
    }

    [[nodiscard]] haven::application::reservations::ReservationListResult find_pending_approvals(
        const haven::domain::OrganizationId& organization_id) const override {
        find_pending_approvals_called_ = true;
        pending_approvals_organization_id_ = organization_id;
        return pending_approvals_result_;
    }

    [[nodiscard]] haven::application::reservations::ReservationListResult find_decided_approvals(
        const haven::domain::OrganizationId& organization_id) const override {
        find_decided_approvals_called_ = true;
        decided_approvals_organization_id_ = organization_id;
        return decided_approvals_result_;
    }

    [[nodiscard]] haven::application::reservations::ReservationListResult find_all(
        const haven::domain::OrganizationId& organization_id) const override {
        find_all_called_ = true;
        all_organization_id_ = organization_id;
        return all_result_;
    }

    [[nodiscard]] haven::application::reservations::ReservationListResult
    find_by_resource_and_interval(const haven::domain::OrganizationId& organization_id,
                                  const haven::domain::ResourceId& resource_id,
                                  const haven::domain::TimeInterval& interval) const override {
        find_by_resource_and_interval_called_ = true;
        calendar_organization_id_ = organization_id;
        calendar_resource_id_ = resource_id;
        calendar_interval_ = interval;
        return calendar_result_;
    }

    [[nodiscard]] bool has_conflict(const haven::domain::OrganizationId& organization_id,
                                    const haven::domain::ResourceId& resource_id,
                                    const haven::domain::TimeInterval& interval) const override {
        has_conflict_called_ = true;
        conflict_organization_ids_.push_back(organization_id);
        conflict_resource_ids_.push_back(resource_id);
        conflict_intervals_.push_back(interval);

        const auto result_index = conflict_resource_ids_.size() - 1U;
        if (result_index < conflict_results_.size()) {
            return conflict_results_.at(result_index);
        }
        return conflict_result_;
    }

    [[nodiscard]] bool has_conflict_excluding(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ResourceId& resource_id,
        const haven::domain::TimeInterval& interval,
        const haven::domain::ReservationId& excluded_reservation_id) const override {
        has_conflict_excluding_called_ = true;
        conflict_excluding_organization_id_ = organization_id;
        conflict_excluding_resource_id_ = resource_id;
        conflict_excluding_interval_ = interval;
        excluded_reservation_id_ = excluded_reservation_id;
        return conflict_excluding_result_;
    }

    [[nodiscard]] haven::application::persistence::PersistenceToken insert(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::Reservation& reservation) override {
        ++insert_call_count_;
        if (lookup_result_.has_value()) {
            throw haven::application::RepositoryError{
                haven::application::RepositoryErrorCode::AlreadyExists,
                "Reservation already exists"};
        }
        ++current_token_;
        saved_organization_id_ = organization_id;
        saved_reservation_ = reservation;
        lookup_result_.emplace(reservation,
                               haven::application::persistence::PersistenceToken{current_token_});
        return haven::application::persistence::PersistenceToken{current_token_};
    }

    [[nodiscard]] haven::application::persistence::PersistenceToken update(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::Reservation& reservation,
        const haven::application::persistence::PersistenceToken& expected_token) override {
        ++update_call_count_;
        last_expected_token_ = expected_token;
        if (force_concurrency_conflict_ ||
            (lookup_result_.has_value() && lookup_result_->persistence_token() != expected_token)) {
            throw haven::application::RepositoryError{
                haven::application::RepositoryErrorCode::ConcurrencyConflict,
                "Reservation persistence token is stale"};
        }
        if (!lookup_result_.has_value()) {
            throw haven::application::RepositoryError{
                haven::application::RepositoryErrorCode::Persistence, "Reservation does not exist"};
        }
        ++current_token_;
        saved_organization_id_ = organization_id;
        saved_reservation_ = reservation;
        lookup_result_.emplace(reservation,
                               haven::application::persistence::PersistenceToken{current_token_});
        return haven::application::persistence::PersistenceToken{current_token_};
    }

    void set_force_concurrency_conflict(const bool value) noexcept {
        force_concurrency_conflict_ = value;
    }

    [[nodiscard]] bool find_by_id_called() const noexcept {
        return find_by_id_called_;
    }
    [[nodiscard]] bool find_by_creator_called() const noexcept {
        return find_by_creator_called_;
    }
    [[nodiscard]] bool find_pending_approvals_called() const noexcept {
        return find_pending_approvals_called_;
    }
    [[nodiscard]] bool find_decided_approvals_called() const noexcept {
        return find_decided_approvals_called_;
    }
    [[nodiscard]] bool find_all_called() const noexcept {
        return find_all_called_;
    }
    [[nodiscard]] bool find_by_resource_and_interval_called() const noexcept {
        return find_by_resource_and_interval_called_;
    }
    [[nodiscard]] bool has_conflict_called() const noexcept {
        return has_conflict_called_;
    }
    [[nodiscard]] bool has_conflict_excluding_called() const noexcept {
        return has_conflict_excluding_called_;
    }
    [[nodiscard]] bool save_called() const noexcept {
        return insert_call_count_ + update_call_count_ != 0;
    }
    [[nodiscard]] std::size_t insert_call_count() const noexcept {
        return insert_call_count_;
    }
    [[nodiscard]] std::size_t update_call_count() const noexcept {
        return update_call_count_;
    }
    [[nodiscard]] const std::optional<haven::application::persistence::PersistenceToken>&
    last_expected_token() const noexcept {
        return last_expected_token_;
    }

    [[nodiscard]] const std::optional<haven::domain::OrganizationId>& lookup_organization_id()
        const noexcept {
        return lookup_organization_id_;
    }
    [[nodiscard]] const std::optional<haven::domain::ReservationId>& lookup_reservation_id()
        const noexcept {
        return lookup_reservation_id_;
    }
    [[nodiscard]] const std::optional<haven::domain::OrganizationId>& creator_organization_id()
        const noexcept {
        return creator_organization_id_;
    }
    [[nodiscard]] const std::optional<haven::domain::UserId>& creator_caller_id() const noexcept {
        return creator_caller_id_;
    }
    [[nodiscard]] const std::optional<haven::domain::OrganizationId>&
    pending_approvals_organization_id() const noexcept {
        return pending_approvals_organization_id_;
    }
    [[nodiscard]] const std::optional<haven::domain::OrganizationId>&
    decided_approvals_organization_id() const noexcept {
        return decided_approvals_organization_id_;
    }
    [[nodiscard]] const std::optional<haven::domain::OrganizationId>& all_organization_id()
        const noexcept {
        return all_organization_id_;
    }
    [[nodiscard]] const std::optional<haven::domain::OrganizationId>& calendar_organization_id()
        const noexcept {
        return calendar_organization_id_;
    }
    [[nodiscard]] const std::optional<haven::domain::ResourceId>& calendar_resource_id()
        const noexcept {
        return calendar_resource_id_;
    }
    [[nodiscard]] const std::optional<haven::domain::TimeInterval>& calendar_interval()
        const noexcept {
        return calendar_interval_;
    }
    [[nodiscard]] const std::vector<haven::domain::OrganizationId>& conflict_organization_ids()
        const noexcept {
        return conflict_organization_ids_;
    }
    [[nodiscard]] const std::vector<haven::domain::ResourceId>& conflict_resource_ids()
        const noexcept {
        return conflict_resource_ids_;
    }
    [[nodiscard]] const std::vector<haven::domain::TimeInterval>& conflict_intervals()
        const noexcept {
        return conflict_intervals_;
    }
    [[nodiscard]] const std::optional<haven::domain::OrganizationId>&
    conflict_excluding_organization_id() const noexcept {
        return conflict_excluding_organization_id_;
    }
    [[nodiscard]] const std::optional<haven::domain::ResourceId>& conflict_excluding_resource_id()
        const noexcept {
        return conflict_excluding_resource_id_;
    }
    [[nodiscard]] const std::optional<haven::domain::TimeInterval>& conflict_excluding_interval()
        const noexcept {
        return conflict_excluding_interval_;
    }
    [[nodiscard]] const std::optional<haven::domain::ReservationId>& excluded_reservation_id()
        const noexcept {
        return excluded_reservation_id_;
    }
    [[nodiscard]] const std::optional<haven::domain::OrganizationId>& saved_organization_id()
        const noexcept {
        return saved_organization_id_;
    }
    [[nodiscard]] const std::optional<haven::domain::Reservation>& saved_reservation()
        const noexcept {
        return saved_reservation_;
    }

private:
    haven::application::reservations::ReservationLookupResult lookup_result_;
    haven::application::reservations::ReservationListResult creator_result_;
    haven::application::reservations::ReservationListResult pending_approvals_result_;
    haven::application::reservations::ReservationListResult decided_approvals_result_;
    haven::application::reservations::ReservationListResult calendar_result_;
    haven::application::reservations::ReservationListResult all_result_;
    bool conflict_result_{false};
    std::vector<bool> conflict_results_;
    bool conflict_excluding_result_{false};
    mutable bool find_by_id_called_{false};
    mutable bool find_by_creator_called_{false};
    mutable bool find_pending_approvals_called_{false};
    mutable bool find_decided_approvals_called_{false};
    mutable bool find_by_resource_and_interval_called_{false};
    mutable bool find_all_called_{false};
    mutable bool has_conflict_called_{false};
    mutable bool has_conflict_excluding_called_{false};
    std::size_t insert_call_count_{0};
    std::size_t update_call_count_{0};
    std::uint64_t current_token_{1};
    bool force_concurrency_conflict_{false};
    std::optional<haven::application::persistence::PersistenceToken> last_expected_token_;
    mutable std::optional<haven::domain::OrganizationId> lookup_organization_id_;
    mutable std::optional<haven::domain::ReservationId> lookup_reservation_id_;
    mutable std::optional<haven::domain::OrganizationId> creator_organization_id_;
    mutable std::optional<haven::domain::UserId> creator_caller_id_;
    mutable std::optional<haven::domain::OrganizationId> all_organization_id_;
    mutable std::optional<haven::domain::OrganizationId> pending_approvals_organization_id_;
    mutable std::optional<haven::domain::OrganizationId> decided_approvals_organization_id_;
    mutable std::optional<haven::domain::OrganizationId> calendar_organization_id_;
    mutable std::optional<haven::domain::ResourceId> calendar_resource_id_;
    mutable std::optional<haven::domain::TimeInterval> calendar_interval_;
    mutable std::vector<haven::domain::OrganizationId> conflict_organization_ids_;
    mutable std::vector<haven::domain::ResourceId> conflict_resource_ids_;
    mutable std::vector<haven::domain::TimeInterval> conflict_intervals_;
    mutable std::optional<haven::domain::OrganizationId> conflict_excluding_organization_id_;
    mutable std::optional<haven::domain::ResourceId> conflict_excluding_resource_id_;
    mutable std::optional<haven::domain::TimeInterval> conflict_excluding_interval_;
    mutable std::optional<haven::domain::ReservationId> excluded_reservation_id_;
    std::optional<haven::domain::OrganizationId> saved_organization_id_;
    std::optional<haven::domain::Reservation> saved_reservation_;
};

}  // namespace haven::tests::util::application

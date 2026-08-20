/**
 * @file reservation_repository.hpp
 * @brief Defines tenant-scoped reservation persistence operations.
 */

#pragma once

#include "haven/application/persistence/loaded.hpp"
#include "haven/domain/reservation.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/time_interval.hpp"
#include "haven/domain/value_objects/user_id.hpp"

#include <optional>
#include <vector>

namespace haven::application::reservations {

/**
 * @brief Represents the result of a tenant-scoped reservation lookup.
 *
 * An empty result means that the reservation either does not exist or is not
 * visible within the supplied organization.
 */
using LoadedReservation = persistence::Loaded<haven::domain::Reservation>;
using ReservationLookupResult = std::optional<LoadedReservation>;

/**
 * @brief Represents reservations returned by an application query.
 */
using ReservationListResult = std::vector<haven::domain::Reservation>;

/**
 * @brief Provides reservation persistence operations required by the application layer.
 *
 * Every repository operation requires explicit organization context.
 * Implementations must never inspect or mutate reservations outside the
 * supplied organization.
 */
class ReservationRepository {
public:
    /**
     * @brief Destroys the reservation repository.
     */
    virtual ~ReservationRepository() = default;

    /**
     * @brief Finds a reservation within an organization.
     *
     * @param organization_id Organization used to scope the lookup.
     * @param reservation_id Reservation identifier to retrieve.
     * @return The visible reservation or an empty result.
     */
    [[nodiscard]] virtual ReservationLookupResult find_by_id(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ReservationId& reservation_id) const = 0;

    /**
     * @brief Finds reservations created by one caller within an organization.
     *
     * @param organization_id Organization used to scope the query.
     * @param caller_id User whose reservations should be returned.
     * @return Reservations belonging to the caller within the organization.
     */
    [[nodiscard]] virtual ReservationListResult find_by_creator(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::UserId& caller_id) const = 0;

    /**
     * @brief Finds reservations awaiting approval within an organization.
     *
     * @param organization_id Organization used to scope the approval queue.
     * @return Pending approval reservations belonging to the organization.
     */
    [[nodiscard]] virtual ReservationListResult find_pending_approvals(
        const haven::domain::OrganizationId& organization_id) const = 0;

    /**
     * @brief Finds reservations previously decided (approved or rejected) within an organization.
     *
     * A reservation is "decided" once it has moved out of pending approval as
     * a direct result of an approval decision: it is either rejected, or
     * confirmed with recorded approval information.
     *
     * @param organization_id Organization used to scope the query.
     * @return Decided reservations belonging to the organization.
     */
    [[nodiscard]] virtual ReservationListResult find_decided_approvals(
        const haven::domain::OrganizationId& organization_id) const = 0;

    /**
     * @brief Finds every reservation within an organization, regardless of status.
     *
     * Intended for administrative visibility across the entire booking history.
     *
     * @param organization_id Organization used to scope the query.
     * @return All reservations belonging to the organization.
     */
    [[nodiscard]] virtual ReservationListResult find_all(
        const haven::domain::OrganizationId& organization_id) const = 0;

    /**
     * @brief Finds reservations for a resource that overlap a requested interval.
     *
     * Calendar views are derived from reservations rather than persisted as a
     * separate availability model.
     *
     * @param organization_id Organization used to scope the query.
     * @param resource_id Resource whose calendar is requested.
     * @param interval Interval used to constrain the calendar view.
     * @return Reservations overlapping the requested interval.
     */
    [[nodiscard]] virtual ReservationListResult find_by_resource_and_interval(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ResourceId& resource_id,
        const haven::domain::TimeInterval& interval) const = 0;

    /**
     * @brief Determines whether a confirmed reservation overlaps an interval.
     *
     * Only reservations that claim the resource schedule participate in this
     * check. Pending and terminal reservations must not produce conflicts.
     *
     * @param organization_id Organization used to scope the conflict check.
     * @param resource_id Resource whose schedule is being checked.
     * @param interval Requested reservation interval.
     * @return true when a blocking reservation overlaps the interval.
     */
    [[nodiscard]] virtual bool has_conflict(const haven::domain::OrganizationId& organization_id,
                                            const haven::domain::ResourceId& resource_id,
                                            const haven::domain::TimeInterval& interval) const = 0;

    /**
     * @brief Counts confirmed reservations that overlap an interval for a resource.
     *
     * Used to support bulk/pooled resources (e.g. a parking lot or a block of
     * shared desks) that can be held by several concurrent reservations up to
     * their total unit capacity, unlike single-unit resources where any
     * overlap is a conflict.
     *
     * The default implementation preserves single-unit semantics by deferring
     * to has_conflict(), so existing implementations do not need to override
     * this method unless they back a repository that actually stores more
     * than one concurrent reservation per resource.
     *
     * @param organization_id Organization used to scope the count.
     * @param resource_id Resource whose schedule is being checked.
     * @param interval Requested reservation interval.
     * @return Number of confirmed reservations overlapping the interval.
     */
    [[nodiscard]] virtual int reserved_unit_count(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ResourceId& resource_id,
        const haven::domain::TimeInterval& interval) const {
        return has_conflict(organization_id, resource_id, interval) ? 1 : 0;
    }

    /**
     * @brief Determines whether an interval conflicts with another reservation.
     *
     * The excluded reservation is ignored so an existing reservation can safely
     * check a replacement interval without conflicting with itself.
     *
     * @param organization_id Organization used to scope the conflict check.
     * @param resource_id Resource whose schedule is being checked.
     * @param interval Proposed replacement interval.
     * @param excluded_reservation_id Reservation excluded from conflict detection.
     * @return true when another blocking reservation overlaps the interval.
     */
    [[nodiscard]] virtual bool has_conflict_excluding(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ResourceId& resource_id,
        const haven::domain::TimeInterval& interval,
        const haven::domain::ReservationId& excluded_reservation_id) const = 0;

    /**
     * @brief Persists a reservation within an organization.
     *
     * @param organization_id Organization that owns the reservation.
     * @param reservation Reservation to persist.
     */
    [[nodiscard]] virtual persistence::PersistenceToken insert(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::Reservation& reservation) = 0;

    /**
     * @brief Replaces a reservation when its persisted revision matches.
     */
    [[nodiscard]] virtual persistence::PersistenceToken update(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::Reservation& reservation,
        const persistence::PersistenceToken& expected_token) = 0;
};

}  // namespace haven::application::reservations

/**
 * @file reservation.hpp
 * @brief Defines the reservation domain aggregate.
 */

#pragma once

#include "haven/domain/events/reservation_domain_event.hpp"
#include "haven/domain/value_objects/approval_info.hpp"
#include "haven/domain/value_objects/cancellation_info.hpp"
#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/purpose.hpp"
#include "haven/domain/value_objects/rejection_info.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
#include "haven/domain/value_objects/reservation_kind.hpp"
#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/time_interval.hpp"
#include "haven/domain/value_objects/user_id.hpp"
#include "haven/domain/value_objects/version.hpp"

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace haven::domain {

/**
 * @brief Represents a fixed-time reservation for one organization resource.
 *
 * Reservation state may change only through named lifecycle operations. The
 * aggregate does not perform repository conflict checks, authorization checks,
 * event serialization, persistence, or external communication.
 */
class Reservation final {
public:
    using TimePoint = std::chrono::system_clock::time_point;

    /**
     * @brief Creates an immediately confirmed reservation.
     */
    [[nodiscard]] static Reservation create_confirmed(OrganizationId organization_id,
                                                      ReservationId reservation_id,
                                                      ResourceId resource_id,
                                                      UserId created_by,
                                                      TimeInterval interval,
                                                      Purpose purpose,
                                                      ReservationKind kind,
                                                      EventId created_event_id,
                                                      EventId confirmed_event_id,
                                                      TimePoint occurred_at);

    /**
     * @brief Creates a reservation awaiting approval.
     */
    [[nodiscard]] static Reservation create_pending_approval(OrganizationId organization_id,
                                                             ReservationId reservation_id,
                                                             ResourceId resource_id,
                                                             UserId created_by,
                                                             TimeInterval interval,
                                                             Purpose purpose,
                                                             ReservationKind kind,
                                                             EventId created_event_id,
                                                             EventId approval_requested_event_id,
                                                             TimePoint occurred_at);

    /**
     * @brief Restores a reservation from previously persisted state.
     *
     * Rehydration preserves the stored lifecycle state, approval information, and
     * optimistic concurrency version. It does not emit domain events because no
     * new business action has occurred.
     *
     * @param organization_id Organization that owns the reservation.
     * @param reservation_id Persisted reservation identifier.
     * @param resource_id Reserved resource identifier.
     * @param created_by User who originally created the reservation.
     * @param interval Persisted reservation interval.
     * @param purpose Persisted free-form purpose.
     * @param kind Persisted reservation kind.
     * @param status Persisted lifecycle status.
     * @param approval_info Persisted approval information, when applicable.
     * @param rejection_info Persisted rejection information, when applicable.
     * @param cancellation_info Persisted cancellation information, when applicable.
     * @param version Persistence-neutral optimistic concurrency version.
     *
     * @return Rehydrated reservation without uncommitted domain events.
     */
    [[nodiscard]] static Reservation rehydrate(OrganizationId organization_id,
                                               ReservationId reservation_id,
                                               ResourceId resource_id,
                                               UserId created_by,
                                               TimeInterval interval,
                                               Purpose purpose,
                                               ReservationKind kind,
                                               ReservationStatus status,
                                               std::optional<ApprovalInfo> approval_info,
                                               std::optional<RejectionInfo> rejection_info,
                                               std::optional<CancellationInfo> cancellation_info,
                                               Version version);

    /**
     * @brief Returns the organization that owns the reservation.
     */
    [[nodiscard]] const OrganizationId& organization_id() const noexcept;

    /**
     * @brief Returns the reservation identifier.
     */
    [[nodiscard]] const ReservationId& reservation_id() const noexcept;

    /**
     * @brief Returns the reserved resource identifier.
     */
    [[nodiscard]] const ResourceId& resource_id() const noexcept;

    /**
     * @brief Returns the user who created the reservation.
     */
    [[nodiscard]] const UserId& created_by() const noexcept;

    /**
     * @brief Returns the reserved interval.
     */
    [[nodiscard]] const TimeInterval& interval() const noexcept;

    /**
     * @brief Returns the free-form reservation purpose.
     */
    [[nodiscard]] const Purpose& purpose() const noexcept;

    /**
     * @brief Returns the reservation kind.
     */
    [[nodiscard]] ReservationKind kind() const noexcept;

    /**
     * @brief Returns the current lifecycle status.
     */
    [[nodiscard]] ReservationStatus status() const noexcept;

    /**
     * @brief Returns approval information when the reservation was approved.
     */
    [[nodiscard]] const std::optional<ApprovalInfo>& approval_info() const noexcept;

    /**
     * @brief Returns rejection information when the reservation was rejected.
     */
    [[nodiscard]] const std::optional<RejectionInfo>& rejection_info() const noexcept;

    /**
     * @brief Returns cancellation information when the reservation was cancelled.
     */
    [[nodiscard]] const std::optional<CancellationInfo>& cancellation_info() const noexcept;

    /**
     * @brief Returns the optimistic concurrency version.
     *
     * @return Persistence-neutral reservation version.
     */
    [[nodiscard]] Version version() const noexcept;

    /**
     * @brief Releases domain events recorded since the previous release.
     *
     * @return Recorded events in the order they occurred.
     */
    [[nodiscard]] std::vector<ReservationDomainEvent> release_domain_events() noexcept;

    /**
     * @brief Approves a pending reservation.
     */
    void approve(UserId approved_by, TimePoint approved_at, EventId confirmed_event_id);

    /**
     * @brief Rejects a pending reservation.
     *
     * @param reason Optional free-form rejection reason supplied by the approver.
     */
    void reject(UserId rejected_by,
                TimePoint rejected_at,
                EventId rejected_event_id,
                std::optional<std::string> reason = std::nullopt);

    /**
     * @brief Cancels a pending or confirmed reservation.
     *
     * @param reason Optional free-form comment supplied by the canceller.
     */
    void cancel(UserId cancelled_by,
                TimePoint cancelled_at,
                EventId cancelled_event_id,
                std::optional<std::string> reason = std::nullopt);

    /**
     * @brief Extends a confirmed reservation to a later end time.
     */
    void extend(TimePoint new_end,
                UserId extended_by,
                TimePoint extended_at,
                EventId extended_event_id);

    /**
     * @brief Marks a pending or confirmed reservation as expired.
     */
    void expire(TimePoint expired_at, EventId expired_event_id);

    /**
     * @brief Marks a confirmed reservation as completed.
     *
     * Completion is system-driven after the reserved interval finishes.
     *
     * @param completed_at Time at which completion occurred.
     * @param completed_event_id Identifier for the completion event.
     *
     * @throws std::logic_error when the reservation is not confirmed.
     */
    void complete(TimePoint completed_at, EventId completed_event_id);

private:
    Reservation(OrganizationId organization_id,
                ReservationId reservation_id,
                ResourceId resource_id,
                UserId created_by,
                TimeInterval interval,
                Purpose purpose,
                ReservationKind kind,
                ReservationStatus status,
                std::optional<ApprovalInfo> approval_info,
                std::optional<RejectionInfo> rejection_info,
                std::optional<CancellationInfo> cancellation_info,
                Version version);

    OrganizationId organization_id_;
    ReservationId reservation_id_;
    ResourceId resource_id_;
    UserId created_by_;
    TimeInterval interval_;
    Purpose purpose_;
    ReservationKind kind_;
    ReservationStatus status_;
    std::optional<ApprovalInfo> approval_info_;
    std::optional<RejectionInfo> rejection_info_;
    std::optional<CancellationInfo> cancellation_info_;
    Version version_;
    std::vector<ReservationDomainEvent> domain_events_;
};

}  // namespace haven::domain

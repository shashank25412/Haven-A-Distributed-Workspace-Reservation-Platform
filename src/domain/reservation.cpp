/**
 * @file reservation.cpp
 * @brief Implements the reservation domain aggregate.
 */

#include "haven/domain/reservation.hpp"

#include "haven/logging/logging.hpp"

#include <optional>
#include <stdexcept>
#include <utility>

namespace haven::domain {

Reservation Reservation::create_confirmed(OrganizationId organization_id,
                                          ReservationId reservation_id,
                                          ResourceId resource_id,
                                          UserId created_by,
                                          TimeInterval interval,
                                          Purpose purpose,
                                          const ReservationKind kind,
                                          EventId created_event_id,
                                          EventId confirmed_event_id,
                                          const TimePoint occurred_at) {
    HVN_TRACE_SCOPE();

    Reservation reservation{std::move(organization_id),
                            std::move(reservation_id),
                            std::move(resource_id),
                            std::move(created_by),
                            std::move(interval),
                            std::move(purpose),
                            kind,
                            ReservationStatus::Confirmed,
                            std::nullopt,
                            std::nullopt,
                            std::nullopt,
                            Version{1}};

    reservation.domain_events_.emplace_back(ReservationCreatedEvent{std::move(created_event_id),
                                                                    occurred_at,
                                                                    reservation.organization_id_,
                                                                    reservation.reservation_id_,
                                                                    reservation.resource_id_,
                                                                    reservation.created_by_,
                                                                    reservation.interval_,
                                                                    reservation.kind_,
                                                                    reservation.status_});

    // Creation and its immediate transition are causally ordered for deterministic Outbox polling.
    reservation.domain_events_.emplace_back(
        ReservationConfirmedEvent{std::move(confirmed_event_id),
                                  occurred_at + TimePoint::duration{1},
                                  reservation.organization_id_,
                                  reservation.reservation_id_,
                                  reservation.resource_id_,
                                  reservation.interval_,
                                  std::nullopt});

    HVN_DEBUG_LOG("Created confirmed reservation and recorded creation and confirmation events.");

    return reservation;
}

Reservation Reservation::create_pending_approval(OrganizationId organization_id,
                                                 ReservationId reservation_id,
                                                 ResourceId resource_id,
                                                 UserId created_by,
                                                 TimeInterval interval,
                                                 Purpose purpose,
                                                 const ReservationKind kind,
                                                 EventId created_event_id,
                                                 EventId approval_requested_event_id,
                                                 const TimePoint occurred_at) {
    HVN_TRACE_SCOPE();

    Reservation reservation{std::move(organization_id),
                            std::move(reservation_id),
                            std::move(resource_id),
                            std::move(created_by),
                            std::move(interval),
                            std::move(purpose),
                            kind,
                            ReservationStatus::PendingApproval,
                            std::nullopt,
                            std::nullopt,
                            std::nullopt,
                            Version{1}};

    reservation.domain_events_.emplace_back(ReservationCreatedEvent{std::move(created_event_id),
                                                                    occurred_at,
                                                                    reservation.organization_id_,
                                                                    reservation.reservation_id_,
                                                                    reservation.resource_id_,
                                                                    reservation.created_by_,
                                                                    reservation.interval_,
                                                                    reservation.kind_,
                                                                    reservation.status_});

    // Creation and its immediate transition are causally ordered for deterministic Outbox polling.
    reservation.domain_events_.emplace_back(
        ReservationApprovalRequestedEvent{std::move(approval_requested_event_id),
                                          occurred_at + TimePoint::duration{1},
                                          reservation.organization_id_,
                                          reservation.reservation_id_,
                                          reservation.resource_id_,
                                          reservation.created_by_,
                                          reservation.interval_,
                                          reservation.kind_});

    HVN_DEBUG_LOG("Created pending reservation and recorded creation and approval-request events.");

    return reservation;
}

Reservation Reservation::rehydrate(OrganizationId organization_id,
                                   ReservationId reservation_id,
                                   ResourceId resource_id,
                                   UserId created_by,
                                   TimeInterval interval,
                                   Purpose purpose,
                                   const ReservationKind kind,
                                   const ReservationStatus status,
                                   std::optional<ApprovalInfo> approval_info,
                                   std::optional<RejectionInfo> rejection_info,
                                   std::optional<CancellationInfo> cancellation_info,
                                   const Version version) {
    HVN_TRACE_SCOPE();

    if (version.value() == 0) {
        throw std::invalid_argument("Persisted reservation version must be greater than zero.");
    }

    Reservation reservation{std::move(organization_id),
                            std::move(reservation_id),
                            std::move(resource_id),
                            std::move(created_by),
                            std::move(interval),
                            std::move(purpose),
                            kind,
                            status,
                            std::move(approval_info),
                            std::move(rejection_info),
                            std::move(cancellation_info),
                            version};

    HVN_DEBUG_LOG("Rehydrated reservation without recording domain events.");

    return reservation;
}

Reservation::Reservation(OrganizationId organization_id,
                         ReservationId reservation_id,
                         ResourceId resource_id,
                         UserId created_by,
                         TimeInterval interval,
                         Purpose purpose,
                         const ReservationKind kind,
                         const ReservationStatus status,
                         std::optional<ApprovalInfo> approval_info,
                         std::optional<RejectionInfo> rejection_info,
                         std::optional<CancellationInfo> cancellation_info,
                         const Version version)
    : organization_id_(std::move(organization_id)),
      reservation_id_(std::move(reservation_id)),
      resource_id_(std::move(resource_id)),
      created_by_(std::move(created_by)),
      interval_(std::move(interval)),
      purpose_(std::move(purpose)),
      kind_(kind),
      status_(status),
      approval_info_(std::move(approval_info)),
      rejection_info_(std::move(rejection_info)),
      cancellation_info_(std::move(cancellation_info)),
      version_(version) {}

const OrganizationId& Reservation::organization_id() const noexcept {
    return organization_id_;
}

const ReservationId& Reservation::reservation_id() const noexcept {
    return reservation_id_;
}

const ResourceId& Reservation::resource_id() const noexcept {
    return resource_id_;
}

const UserId& Reservation::created_by() const noexcept {
    return created_by_;
}

const TimeInterval& Reservation::interval() const noexcept {
    return interval_;
}

const Purpose& Reservation::purpose() const noexcept {
    return purpose_;
}

ReservationKind Reservation::kind() const noexcept {
    return kind_;
}

ReservationStatus Reservation::status() const noexcept {
    return status_;
}

const std::optional<ApprovalInfo>& Reservation::approval_info() const noexcept {
    return approval_info_;
}

const std::optional<RejectionInfo>& Reservation::rejection_info() const noexcept {
    return rejection_info_;
}

const std::optional<CancellationInfo>& Reservation::cancellation_info() const noexcept {
    return cancellation_info_;
}

Version Reservation::version() const noexcept {
    return version_;
}

std::vector<ReservationDomainEvent> Reservation::release_domain_events() noexcept {
    return std::exchange(domain_events_, {});
}

void Reservation::approve(UserId approved_by,
                          const TimePoint approved_at,
                          EventId confirmed_event_id) {
    HVN_TRACE_SCOPE();

    if (status_ != ReservationStatus::PendingApproval) {
        HVN_WARN_LOG(
            "Reservation approval denied because the reservation is not pending approval.");
        throw std::logic_error("Only a pending reservation may be approved.");
    }

    approval_info_.emplace(std::move(approved_by), approved_at);
    status_ = ReservationStatus::Confirmed;
    version_ = Version{version_.value() + 1};

    domain_events_.emplace_back(ReservationConfirmedEvent{std::move(confirmed_event_id),
                                                          approved_at,
                                                          organization_id_,
                                                          reservation_id_,
                                                          resource_id_,
                                                          interval_,
                                                          approval_info_->approved_by()});

    HVN_DEBUG_LOG("Reservation transitioned from pending approval to confirmed.");
}

void Reservation::reject(UserId rejected_by,
                         const TimePoint rejected_at,
                         EventId rejected_event_id,
                         std::optional<std::string> reason) {
    HVN_TRACE_SCOPE();

    if (status_ != ReservationStatus::PendingApproval) {
        HVN_WARN_LOG(
            "Reservation rejection denied because the reservation is not pending approval.");
        throw std::logic_error("Only a pending reservation may be rejected.");
    }

    rejection_info_.emplace(rejected_by, rejected_at, std::move(reason));
    status_ = ReservationStatus::Rejected;
    version_ = Version{version_.value() + 1};

    domain_events_.emplace_back(ReservationRejectedEvent{std::move(rejected_event_id),
                                                         rejected_at,
                                                         organization_id_,
                                                         reservation_id_,
                                                         resource_id_,
                                                         std::move(rejected_by)});

    HVN_DEBUG_LOG("Reservation transitioned from pending approval to rejected.");
}

void Reservation::cancel(UserId cancelled_by,
                         const TimePoint cancelled_at,
                         EventId cancelled_event_id,
                         std::optional<std::string> reason) {
    HVN_TRACE_SCOPE();

    if (is_terminal(status_)) {
        HVN_WARN_LOG(
            "Reservation cancellation denied because the reservation is already terminal.");
        throw std::logic_error("A terminal reservation cannot be cancelled.");
    }

    const ReservationStatus previous_status = status_;
    status_ = ReservationStatus::Cancelled;
    cancellation_info_.emplace(cancelled_by, cancelled_at, reason);
    version_ = Version{version_.value() + 1};

    domain_events_.emplace_back(ReservationCancelledEvent{std::move(cancelled_event_id),
                                                          cancelled_at,
                                                          organization_id_,
                                                          reservation_id_,
                                                          resource_id_,
                                                          std::move(cancelled_by),
                                                          previous_status,
                                                          std::move(reason)});

    HVN_DEBUG_LOG("Reservation transitioned to cancelled.");
}

void Reservation::extend(const TimePoint new_end,
                         UserId extended_by,
                         const TimePoint extended_at,
                         EventId extended_event_id) {
    HVN_TRACE_SCOPE();

    if (status_ != ReservationStatus::Confirmed) {
        HVN_WARN_LOG("Reservation extension denied because the reservation is not confirmed.");
        throw std::logic_error("Only a confirmed reservation may be extended.");
    }

    if (new_end <= interval_.end()) {
        HVN_WARN_LOG(
            "Reservation extension denied because the new end time does not move forward.");
        throw std::invalid_argument("Reservation extension must move the end time forward.");
    }

    const TimeInterval previous_interval = interval_;
    const TimeInterval extended_interval{interval_.start(), new_end};

    domain_events_.emplace_back(ReservationExtendedEvent{std::move(extended_event_id),
                                                         extended_at,
                                                         organization_id_,
                                                         reservation_id_,
                                                         resource_id_,
                                                         std::move(extended_by),
                                                         previous_interval,
                                                         extended_interval});

    interval_ = extended_interval;
    version_ = Version{version_.value() + 1};

    HVN_DEBUG_LOG("Confirmed reservation interval was extended.");
}

void Reservation::expire(const TimePoint expired_at, EventId expired_event_id) {
    HVN_TRACE_SCOPE();

    if (is_terminal(status_)) {
        HVN_WARN_LOG("Reservation expiration denied because the reservation is already terminal.");
        throw std::logic_error("A terminal reservation cannot be expired.");
    }

    const ReservationStatus previous_status = status_;
    status_ = ReservationStatus::Expired;
    version_ = Version{version_.value() + 1};

    domain_events_.emplace_back(ReservationExpiredEvent{std::move(expired_event_id),
                                                        expired_at,
                                                        organization_id_,
                                                        reservation_id_,
                                                        resource_id_,
                                                        previous_status});

    HVN_DEBUG_LOG("Reservation transitioned to expired.");
}

void Reservation::complete(const TimePoint completed_at, EventId completed_event_id) {
    HVN_TRACE_SCOPE();

    if (status_ != ReservationStatus::Confirmed) {
        HVN_WARN_LOG("Reservation completion denied because the reservation is not confirmed.");
        throw std::logic_error("Only a confirmed reservation may be completed.");
    }

    domain_events_.emplace_back(ReservationCompletedEvent{std::move(completed_event_id),
                                                          completed_at,
                                                          organization_id_,
                                                          reservation_id_,
                                                          resource_id_,
                                                          interval_});

    status_ = ReservationStatus::Completed;
    version_ = Version{version_.value() + 1};

    HVN_DEBUG_LOG("Reservation transitioned from confirmed to completed.");
}

}  // namespace haven::domain

/**
 * @file create_reservation_fingerprint_input.hpp
 * @brief Defines the logical payload used to fingerprint reservation creation.
 */

#pragma once

#include "haven/application/idempotency/idempotency_fingerprint.hpp"
#include "haven/domain/value_objects/purpose.hpp"
#include "haven/domain/value_objects/reservation_kind.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/time_interval.hpp"
#include "haven/domain/value_objects/user_id.hpp"

#include <utility>

namespace haven::application::idempotency {

/**
 * @brief Contains only caller and authorization inputs defining a logical create request.
 *
 * Server-generated reservation IDs, event IDs, timestamps, domain versions,
 * and persistence revisions are deliberately excluded.
 */
class CreateReservationFingerprintInput final {
public:
    CreateReservationFingerprintInput(haven::domain::ResourceId resource_id,
                                      haven::domain::UserId creator_id,
                                      haven::domain::TimeInterval interval,
                                      haven::domain::Purpose purpose,
                                      haven::domain::ReservationKind reservation_kind,
                                      bool maintenance_authorized)
        : resource_id_(std::move(resource_id)),
          creator_id_(std::move(creator_id)),
          interval_(std::move(interval)),
          purpose_(std::move(purpose)),
          reservation_kind_(reservation_kind),
          maintenance_authorized_(maintenance_authorized) {}

    [[nodiscard]] const haven::domain::ResourceId& resource_id() const noexcept {
        return resource_id_;
    }
    [[nodiscard]] const haven::domain::UserId& creator_id() const noexcept {
        return creator_id_;
    }
    [[nodiscard]] const haven::domain::TimeInterval& interval() const noexcept {
        return interval_;
    }
    [[nodiscard]] const haven::domain::Purpose& purpose() const noexcept {
        return purpose_;
    }
    [[nodiscard]] haven::domain::ReservationKind reservation_kind() const noexcept {
        return reservation_kind_;
    }
    [[nodiscard]] bool maintenance_authorized() const noexcept {
        return maintenance_authorized_;
    }

private:
    haven::domain::ResourceId resource_id_;
    haven::domain::UserId creator_id_;
    haven::domain::TimeInterval interval_;
    haven::domain::Purpose purpose_;
    haven::domain::ReservationKind reservation_kind_;
    bool maintenance_authorized_;
};

/**
 * @brief Calculates SHA-256 over Haven's version-one canonical binary encoding.
 *
 * Strings use an unsigned 64-bit big-endian byte length followed by exact
 * bytes. Timestamps use signed UTC nanoseconds since the Unix epoch encoded as
 * big-endian two's-complement bits. The boolean is one byte, and fields have a
 * fixed order independent of JSON and locale. The result is lowercase hex.
 */
[[nodiscard]] IdempotencyFingerprint create_reservation_fingerprint(
    const CreateReservationFingerprintInput& input);

}  // namespace haven::application::idempotency

/**
 * @file reservation_document_mapper.hpp
 * @brief Declares reservation persistence mapping.
 */

#pragma once

#include "haven/domain/reservation.hpp"
#include "haven/infrastructure/persistence/couchbase/reservation_document.hpp"

namespace haven::infrastructure::persistence::couchbase {

/** @brief Maps a reservation aggregate to a validated persistence document. */
[[nodiscard]] ReservationDocument to_reservation_document(const domain::Reservation& reservation);

/**
 * @brief Restores an existing reservation from persisted state.
 *
 * This operation preserves purpose text and lifecycle state exactly and does
 * not perform a new reservation creation operation.
 */
[[nodiscard]] domain::Reservation to_domain_reservation(const ReservationDocument& document);

}  // namespace haven::infrastructure::persistence::couchbase

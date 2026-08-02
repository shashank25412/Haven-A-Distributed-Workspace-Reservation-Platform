/**
 * @file couchbase_document_key.hpp
 * @brief Declares tenant-scoped Couchbase document-key generation.
 */

#pragma once

#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"

#include <string>

namespace haven::infrastructure::persistence::couchbase {

/**
 * @brief Creates the tenant-scoped Couchbase document key for a resource.
 *
 * @param organization_id Organization that owns the resource.
 * @param resource_id Resource identifier.
 * @return Tenant-scoped resource document key.
 */
[[nodiscard]] std::string resource_document_key(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ResourceId& resource_id);

/**
 * @brief Creates the tenant-scoped Couchbase document key for a reservation.
 *
 * @param organization_id Organization that owns the reservation.
 * @param reservation_id Reservation identifier.
 * @return Tenant-scoped reservation document key.
 */
[[nodiscard]] std::string reservation_document_key(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ReservationId& reservation_id);

/** @brief Creates the tenant-scoped Couchbase document key for an Outbox event. */
[[nodiscard]] std::string outbox_document_key(const haven::domain::OrganizationId& organization_id,
                                              const haven::domain::EventId& event_id);

}  // namespace haven::infrastructure::persistence::couchbase

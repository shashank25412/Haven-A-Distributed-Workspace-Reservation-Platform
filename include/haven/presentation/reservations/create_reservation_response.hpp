/**
 * @file create_reservation_response.hpp
 * @brief Declares the reservation creation HTTP response DTO.
 */

#pragma once

#include "haven/application/reservations/create_reservation_result.hpp"

#include <json/value.h>

namespace haven::presentation::reservations {

class CreateReservationResponse final {
public:
    explicit CreateReservationResponse(
        const haven::application::reservations::CreateReservationResult& result);
    [[nodiscard]] Json::Value to_json() const;

private:
    Json::Value response_;
};

}  // namespace haven::presentation::reservations

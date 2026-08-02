#include "haven/presentation/reservations/create_reservation_response.hpp"

#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/presentation/reservations/create_reservation_request.hpp"

#include <stdexcept>
#include <string>

namespace haven::presentation::reservations {

CreateReservationResponse::CreateReservationResponse(
    const haven::application::reservations::CreateReservationResult& result) {
    if (!result.reservation() || !result.created_at())
        throw std::invalid_argument("Successful reservation response requires complete result");
    const auto& reservation = *result.reservation();
    response_["reservationId"] = reservation.reservation_id().value();
    response_["resourceId"] = reservation.resource_id().value();
    response_["status"] = std::string{haven::domain::to_string(reservation.status())};
    response_["startTime"] = reservation_http_timestamp(reservation.interval().start());
    response_["endTime"] = reservation_http_timestamp(reservation.interval().end());
    response_["createdAt"] = reservation_http_timestamp(*result.created_at());
}

Json::Value CreateReservationResponse::to_json() const {
    return response_;
}

}  // namespace haven::presentation::reservations

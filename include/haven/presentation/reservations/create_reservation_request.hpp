#pragma once

#include "haven/domain/value_objects/purpose.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/time_interval.hpp"

#include <json/value.h>

namespace haven::presentation::reservations {

class CreateReservationRequest final {
public:
    [[nodiscard]] static CreateReservationRequest from_json(const Json::Value& json);
    [[nodiscard]] const haven::domain::ResourceId& resource_id() const noexcept;
    [[nodiscard]] const haven::domain::TimeInterval& interval() const noexcept;
    [[nodiscard]] const haven::domain::Purpose& purpose() const noexcept;

private:
    CreateReservationRequest(haven::domain::ResourceId resource_id,
                             haven::domain::TimeInterval interval,
                             haven::domain::Purpose purpose);
    haven::domain::ResourceId resource_id_;
    haven::domain::TimeInterval interval_;
    haven::domain::Purpose purpose_;
};

[[nodiscard]] std::string reservation_http_timestamp(
    std::chrono::system_clock::time_point timestamp);

}  // namespace haven::presentation::reservations

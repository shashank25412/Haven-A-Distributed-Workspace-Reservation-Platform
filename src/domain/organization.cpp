/**
 * @file organization.cpp
 * @brief Implements the organization directory aggregate.
 */

#include "haven/domain/organization.hpp"

#include <stdexcept>
#include <utility>

namespace haven::domain {

Organization::Organization(OrganizationId organization_id, std::string name, std::string image_url,
                            int rank)
    : organization_id_(std::move(organization_id)),
      name_(std::move(name)),
      image_url_(std::move(image_url)),
      rank_(rank) {
    if (name_.empty()) {
        throw std::invalid_argument("Organization name must not be empty.");
    }
}

const OrganizationId& Organization::organization_id() const noexcept {
    return organization_id_;
}

const std::string& Organization::name() const noexcept {
    return name_;
}

const std::string& Organization::image_url() const noexcept {
    return image_url_;
}

int Organization::rank() const noexcept {
    return rank_;
}

}  // namespace haven::domain

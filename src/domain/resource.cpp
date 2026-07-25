/**
 * @file resource.cpp
 * @brief Implements the reservable resource domain aggregate.
 */

#include "haven/domain/resource.hpp"

#include <utility>

namespace haven::domain {

Resource::Resource(
    OrganizationId organization_id,
    ResourceId resource_id,
    const ResourceType type,
    const ResourceStatus status,
    const bool requires_approval)
    : organization_id_(std::move(organization_id)),
      resource_id_(std::move(resource_id)),
      type_(type),
      status_(status),
      requires_approval_(requires_approval) {
}

const OrganizationId& Resource::organization_id() const noexcept {
    return organization_id_;
}

const ResourceId& Resource::resource_id() const noexcept {
    return resource_id_;
}

ResourceType Resource::type() const noexcept {
    return type_;
}

ResourceStatus Resource::status() const noexcept {
    return status_;
}

bool Resource::is_active() const noexcept {
    return status_ == ResourceStatus::Active;
}

bool Resource::requires_approval() const noexcept {
    return requires_approval_;
}

void Resource::activate() noexcept {
    status_ = ResourceStatus::Active;
}

void Resource::deactivate() noexcept {
    status_ = ResourceStatus::Inactive;
}

}  // namespace haven::domain
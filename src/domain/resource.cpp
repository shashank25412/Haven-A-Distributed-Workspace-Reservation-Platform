/**
 * @file resource.cpp
 * @brief Implements the reservable resource domain aggregate.
 */

#include "haven/domain/resource.hpp"

#include "haven/logging/logging.hpp"

#include <stdexcept>
#include <utility>

namespace haven::domain {

Resource::Resource(OrganizationId organization_id,
                   ResourceId resource_id,
                   const ResourceType type,
                   const ResourceStatus status,
                   const bool requires_approval,
                   const std::uint32_t total_units,
                   std::string address)
    : organization_id_(std::move(organization_id)),
      resource_id_(std::move(resource_id)),
      name_(),
      description_(),
      type_(type),
      status_(status),
      requires_approval_(requires_approval),
      version_(Version{1}),
      total_units_(total_units == 0 ? 1 : total_units),
      address_(std::move(address)) {}

Resource Resource::rehydrate(OrganizationId organization_id,
                             ResourceId resource_id,
                             std::string name,
                             std::string description,
                             const ResourceType type,
                             const ResourceStatus status,
                             const bool requires_approval,
                             const Version version,
                             const std::uint32_t total_units,
                             std::string address) {
    HVN_TRACE_SCOPE();

    if (name.empty()) {
        throw std::invalid_argument("Resource name must not be empty.");
    }
    if (version.value() == 0) {
        throw std::invalid_argument("Persisted resource version must be greater than zero.");
    }

    return Resource{std::move(organization_id),
                    std::move(resource_id),
                    std::move(name),
                    std::move(description),
                    type,
                    status,
                    requires_approval,
                    version,
                    total_units == 0 ? 1 : total_units,
                    std::move(address)};
}

Resource::Resource(OrganizationId organization_id,
                   ResourceId resource_id,
                   std::string name,
                   std::string description,
                   const ResourceType type,
                   const ResourceStatus status,
                   const bool requires_approval,
                   const Version version,
                   const std::uint32_t total_units,
                   std::string address)
    : organization_id_(std::move(organization_id)),
      resource_id_(std::move(resource_id)),
      name_(std::move(name)),
      description_(std::move(description)),
      type_(type),
      status_(status),
      requires_approval_(requires_approval),
      version_(version),
      total_units_(total_units),
      address_(std::move(address)) {}

const OrganizationId& Resource::organization_id() const noexcept {
    return organization_id_;
}

const ResourceId& Resource::resource_id() const noexcept {
    return resource_id_;
}

const std::string& Resource::name() const noexcept {
    return name_;
}

const std::string& Resource::description() const noexcept {
    return description_;
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

Version Resource::version() const noexcept {
    return version_;
}

std::uint32_t Resource::total_units() const noexcept {
    return total_units_;
}

const std::string& Resource::address() const noexcept {
    return address_;
}

void Resource::activate() noexcept {
    if (status_ == ResourceStatus::Active) {
        return;
    }
    status_ = ResourceStatus::Active;
    version_ = Version{version_.value() + 1};
}

void Resource::deactivate() noexcept {
    if (status_ == ResourceStatus::Inactive) {
        return;
    }
    status_ = ResourceStatus::Inactive;
    version_ = Version{version_.value() + 1};
}

}  // namespace haven::domain

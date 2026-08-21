/**
 * @file update_resource_command.hpp
 * @brief Defines the command for updating an existing resource.
 */

#pragma once

#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/resource_status.hpp"
#include "haven/domain/value_objects/resource_type.hpp"

#include <cstdint>
#include <string>

namespace haven::application::resources {

class UpdateResourceCommand final {
public:
    UpdateResourceCommand(haven::domain::OrganizationId organization_id,
                          haven::domain::ResourceId resource_id,
                          std::string name,
                          std::string description,
                          haven::domain::ResourceType resource_type,
                          haven::domain::ResourceStatus status,
                          bool requires_approval,
                          std::uint32_t total_units,
                          std::string address)
        : organization_id_(std::move(organization_id)),
          resource_id_(std::move(resource_id)),
          name_(std::move(name)),
          description_(std::move(description)),
          resource_type_(resource_type),
          status_(status),
          requires_approval_(requires_approval),
          total_units_(total_units == 0 ? 1 : total_units),
          address_(std::move(address)) {}

    [[nodiscard]] const haven::domain::OrganizationId& organization_id() const noexcept {
        return organization_id_;
    }
    [[nodiscard]] const haven::domain::ResourceId& resource_id() const noexcept {
        return resource_id_;
    }
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const std::string& description() const noexcept { return description_; }
    [[nodiscard]] haven::domain::ResourceType resource_type() const noexcept { return resource_type_; }
    [[nodiscard]] haven::domain::ResourceStatus status() const noexcept { return status_; }
    [[nodiscard]] bool requires_approval() const noexcept { return requires_approval_; }
    [[nodiscard]] std::uint32_t total_units() const noexcept { return total_units_; }
    [[nodiscard]] const std::string& address() const noexcept { return address_; }

private:
    haven::domain::OrganizationId organization_id_;
    haven::domain::ResourceId resource_id_;
    std::string name_;
    std::string description_;
    haven::domain::ResourceType resource_type_;
    haven::domain::ResourceStatus status_;
    bool requires_approval_;
    std::uint32_t total_units_;
    std::string address_;
};

}  // namespace haven::application::resources

/**
 * @file create_resource_handler.hpp
 * @brief Declares the CreateResource application use-case handler.
 */

#pragma once

#include "haven/application/resources/create_resource_command.hpp"
#include "haven/application/resources/resource_repository.hpp"
#include "haven/domain/resource.hpp"

namespace haven::application::resources {

/**
 * @brief Creates and persists a new resource aggregate.
 */
class CreateResourceHandler final {
public:
    explicit CreateResourceHandler(ResourceRepository& resource_repository) noexcept;

    /**
     * @brief Executes the resource creation.
     *
     * @param command Fields for the new resource.
     * @return The newly created resource aggregate.
     * @throws std::invalid_argument If name is empty.
     * @throws RepositoryError If persistence fails or a duplicate key exists.
     */
    haven::domain::Resource handle(const CreateResourceCommand& command) const;

private:
    ResourceRepository& resource_repository_;
};

}  // namespace haven::application::resources

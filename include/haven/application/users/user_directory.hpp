/**
 * @file user_directory.hpp
 * @brief Declares the port for resolving a user's display name by identifier.
 */

#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace haven::application::users {

/**
 * @brief Resolves human-readable display names for user identifiers.
 */
class UserDirectory {
public:
    virtual ~UserDirectory() = default;

    /**
     * @brief Looks up the display name for a user identifier.
     *
     * @param user_id Identifier of the user to resolve.
     * @return The display name, or std::nullopt if no matching account exists.
     */
    [[nodiscard]] virtual std::optional<std::string> find_display_name(
        std::string_view user_id) const = 0;
};

}  // namespace haven::application::users

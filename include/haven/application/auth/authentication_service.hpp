#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace haven::application::auth {

enum class AuthenticationErrorCode {
    invalid_input,
    account_not_found,
    account_exists,
    invalid_credentials,
    invalid_session,
    persistence,
};

class AuthenticationError final : public std::runtime_error {
public:
    AuthenticationError(AuthenticationErrorCode code, std::string message)
        : std::runtime_error(std::move(message)), code_(code) {}

    [[nodiscard]] AuthenticationErrorCode code() const noexcept { return code_; }

private:
    AuthenticationErrorCode code_;
};

struct AuthenticatedAccount final {
    std::string user_id;
    std::string email;
    std::string organization_id;
    std::string role;
    std::string access_token;
};

class AuthenticationService {
public:
    virtual ~AuthenticationService() = default;
    [[nodiscard]] virtual AuthenticatedAccount sign_up(std::string_view email,
                                                       std::string_view password) const = 0;
    [[nodiscard]] virtual AuthenticatedAccount login(std::string_view email,
                                                     std::string_view password) const = 0;
    [[nodiscard]] virtual AuthenticatedAccount authenticate(
        std::string_view access_token) const = 0;
    virtual void logout(std::string_view access_token) const = 0;
};

}  // namespace haven::application::auth

#pragma once

#include "haven/application/auth/authentication_service.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace haven::infrastructure::persistence::couchbase {

class CouchbaseConnection;

class CouchbaseAuthenticationService final
    : public haven::application::auth::AuthenticationService {
public:
    CouchbaseAuthenticationService(std::shared_ptr<CouchbaseConnection> connection,
                                   std::string default_organization_id);

    [[nodiscard]] haven::application::auth::AuthenticatedAccount sign_up(
        std::string_view email,
        std::string_view password,
        std::string_view name,
        std::string_view contact_number) const override;
    [[nodiscard]] haven::application::auth::AuthenticatedAccount login(
        std::string_view email,
        std::string_view password) const override;
    [[nodiscard]] haven::application::auth::AuthenticatedAccount authenticate(
        std::string_view access_token) const override;
    void logout(std::string_view access_token) const override;

private:
    std::shared_ptr<CouchbaseConnection> connection_;
    std::string default_organization_id_;
};

}  // namespace haven::infrastructure::persistence::couchbase

#include "haven/infrastructure/persistence/couchbase/couchbase_authentication_service.hpp"

#include "haven/infrastructure/persistence/couchbase/couchbase_connection.hpp"
#include "haven/logging/logging.hpp"

#include <couchbase/codec/tao_json_serializer.hxx>
#include <couchbase/error_codes.hxx>
#include <couchbase/insert_options.hxx>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <tao/json.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace haven::infrastructure::persistence::couchbase {
using AuthenticationError = haven::application::auth::AuthenticationError;
using AuthenticationErrorCode = haven::application::auth::AuthenticationErrorCode;
using AuthenticatedAccount = haven::application::auth::AuthenticatedAccount;
namespace {

constexpr std::string_view kCredentialsCollection{"credentials"};
constexpr std::string_view kSessionsCollection{"sessions"};
constexpr std::uint64_t kScryptN = 32768U;
constexpr std::uint64_t kScryptR = 8U;
constexpr std::uint64_t kScryptP = 1U;
constexpr std::uint64_t kScryptMaximumMemory = 64U * 1024U * 1024U;
constexpr std::size_t kSaltBytes = 16U;
constexpr std::size_t kHashBytes = 32U;
constexpr std::size_t kTokenBytes = 32U;
constexpr auto kSessionLifetime = std::chrono::hours{8};

[[nodiscard]] std::string hex_encode(const unsigned char* bytes, const std::size_t size) {
    constexpr char digits[] = "0123456789abcdef";
    auto encoded = std::string(size * 2U, '0');
    for (std::size_t index = 0; index < size; ++index) {
        encoded[index * 2U] = digits[(bytes[index] >> 4U) & 0x0fU];
        encoded[index * 2U + 1U] = digits[bytes[index] & 0x0fU];
    }
    return encoded;
}

[[nodiscard]] std::vector<unsigned char> hex_decode(const std::string_view encoded) {
    if (encoded.size() % 2U != 0U) {
        throw AuthenticationError{AuthenticationErrorCode::persistence,
                                  "Stored credential encoding is invalid"};
    }
    const auto nibble = [](const char character) -> unsigned char {
        if (character >= '0' && character <= '9')
            return static_cast<unsigned char>(character - '0');
        if (character >= 'a' && character <= 'f')
            return static_cast<unsigned char>(character - 'a' + 10);
        throw AuthenticationError{AuthenticationErrorCode::persistence,
                                  "Stored credential encoding is invalid"};
    };
    auto decoded = std::vector<unsigned char>(encoded.size() / 2U);
    for (std::size_t index = 0; index < decoded.size(); ++index) {
        decoded[index] = static_cast<unsigned char>(
            (nibble(encoded[index * 2U]) << 4U) | nibble(encoded[index * 2U + 1U]));
    }
    return decoded;
}

template <std::size_t Size>
[[nodiscard]] std::array<unsigned char, Size> random_bytes() {
    auto bytes = std::array<unsigned char, Size>{};
    if (RAND_bytes(bytes.data(), static_cast<int>(bytes.size())) != 1) {
        throw AuthenticationError{AuthenticationErrorCode::persistence,
                                  "Secure random generation failed"};
    }
    return bytes;
}

[[nodiscard]] std::array<unsigned char, kHashBytes> sha256(const std::string_view value) {
    auto digest = std::array<unsigned char, kHashBytes>{};
    unsigned int digest_size = 0U;
    if (EVP_Digest(value.data(), value.size(), digest.data(), &digest_size, EVP_sha256(), nullptr) !=
            1 ||
        digest_size != digest.size()) {
        throw AuthenticationError{AuthenticationErrorCode::persistence,
                                  "Credential digest generation failed"};
    }
    return digest;
}

[[nodiscard]] std::array<unsigned char, kHashBytes> password_hash(
    const std::string_view password,
    const unsigned char* salt,
    const std::size_t salt_size) {
    auto hash = std::array<unsigned char, kHashBytes>{};
    if (EVP_PBE_scrypt(password.data(),
                       password.size(),
                       salt,
                       salt_size,
                       kScryptN,
                       kScryptR,
                       kScryptP,
                       kScryptMaximumMemory,
                       hash.data(),
                       hash.size()) != 1) {
        throw AuthenticationError{AuthenticationErrorCode::persistence,
                                  "Password hashing failed"};
    }
    return hash;
}

[[nodiscard]] std::string normalize_email(const std::string_view email) {
    const auto first = email.find_first_not_of(" \t\r\n");
    const auto last = email.find_last_not_of(" \t\r\n");
    if (first == std::string_view::npos) return {};
    auto normalized = std::string{email.substr(first, last - first + 1U)};
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char value) {
        return static_cast<char>(std::tolower(value));
    });
    return normalized;
}

void validate_credentials(const std::string_view email, const std::string_view password) {
    const auto at = email.find('@');
    if (email.size() > 254U || at == std::string_view::npos || at == 0U ||
        at == email.size() - 1U || email.find('@', at + 1U) != std::string_view::npos) {
        throw AuthenticationError{AuthenticationErrorCode::invalid_input,
                                  "A valid email address is required"};
    }
    if (password.size() < 8U || password.size() > 128U) {
        throw AuthenticationError{AuthenticationErrorCode::invalid_input,
                                  "Password must contain between 8 and 128 characters"};
    }
}

[[nodiscard]] std::string credential_key(const std::string_view email) {
    const auto digest = sha256(email);
    return "credential::" + hex_encode(digest.data(), digest.size());
}

[[nodiscard]] std::string session_key(const std::string_view access_token) {
    const auto digest = sha256(access_token);
    return "session::" + hex_encode(digest.data(), digest.size());
}

[[nodiscard]] std::uint64_t now_epoch_seconds() {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
                                         std::chrono::system_clock::now().time_since_epoch())
                                         .count());
}

[[nodiscard]] AuthenticatedAccount account_from_document(const tao::json::value& document) {
    return AuthenticatedAccount{
        .user_id = document.at("userId").get_string(),
        .email = document.at("email").get_string(),
        .organization_id = document.at("organizationId").get_string(),
        .role = document.at("role").get_string(),
        .access_token = {},
    };
}

[[nodiscard]] AuthenticatedAccount create_session(
    const std::shared_ptr<CouchbaseConnection>& connection,
    AuthenticatedAccount account) {
    const auto token_bytes = random_bytes<kTokenBytes>();
    account.access_token = "hvn_" + hex_encode(token_bytes.data(), token_bytes.size());
    const auto expires_at =
        now_epoch_seconds() +
        static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(kSessionLifetime).count());
    const auto document = tao::json::value{
        {"documentType", "authenticationSession"},
        {"schemaVersion", 1U},
        {"userId", account.user_id},
        {"email", account.email},
        {"organizationId", account.organization_id},
        {"role", account.role},
        {"expiresAt", expires_at},
    };
    auto options = ::couchbase::insert_options{};
    options.expiry(std::chrono::duration_cast<std::chrono::seconds>(kSessionLifetime));
    auto [error, result] = connection->collection(kSessionsCollection)
                               .insert(session_key(account.access_token), document, options)
                               .get();
    static_cast<void>(result);
    if (error) {
        HVN_ERROR_LOG("Couchbase authentication session insert failed: ", error.ec().message());
        throw AuthenticationError{AuthenticationErrorCode::persistence,
                                  "Authentication session could not be created"};
    }
    return account;
}

}  // namespace

CouchbaseAuthenticationService::CouchbaseAuthenticationService(
    std::shared_ptr<CouchbaseConnection> connection,
    std::string default_organization_id)
    : connection_(std::move(connection)),
      default_organization_id_(std::move(default_organization_id)) {
    if (!connection_ || default_organization_id_.empty()) {
        throw std::invalid_argument("Authentication service configuration is invalid");
    }
}

AuthenticatedAccount CouchbaseAuthenticationService::sign_up(const std::string_view email,
                                                              const std::string_view password) const {
    const auto normalized_email = normalize_email(email);
    validate_credentials(normalized_email, password);
    const auto salt = random_bytes<kSaltBytes>();
    const auto hash = password_hash(password, salt.data(), salt.size());
    const auto identifier = random_bytes<16U>();
    const auto document = tao::json::value{
        {"documentType", "userCredential"},
        {"schemaVersion", 1U},
        {"userId", "usr_" + hex_encode(identifier.data(), identifier.size())},
        {"email", normalized_email},
        {"organizationId", default_organization_id_},
        {"role", "MEMBER"},
        {"passwordAlgorithm", "scrypt"},
        {"passwordSalt", hex_encode(salt.data(), salt.size())},
        {"passwordHash", hex_encode(hash.data(), hash.size())},
        {"scryptN", kScryptN},
        {"scryptR", kScryptR},
        {"scryptP", kScryptP},
    };
    auto [error, result] = connection_->collection(kCredentialsCollection)
                               .insert(credential_key(normalized_email), document)
                               .get();
    static_cast<void>(result);
    if (error.ec() == ::couchbase::errc::key_value::document_exists) {
        throw AuthenticationError{AuthenticationErrorCode::account_exists,
                                  "An account already exists for this email"};
    }
    if (error) {
        HVN_ERROR_LOG("Couchbase credential insert failed: ", error.ec().message());
        throw AuthenticationError{AuthenticationErrorCode::persistence,
                                  "Account could not be created"};
    }
    return create_session(connection_, account_from_document(document));
}

AuthenticatedAccount CouchbaseAuthenticationService::login(const std::string_view email,
                                                            const std::string_view password) const {
    const auto normalized_email = normalize_email(email);
    validate_credentials(normalized_email, password);
    auto [error, result] = connection_->collection(kCredentialsCollection)
                               .get(credential_key(normalized_email))
                               .get();
    if (error.ec() == ::couchbase::errc::key_value::document_not_found) {
        throw AuthenticationError{AuthenticationErrorCode::account_not_found,
                                  "No account exists for this email"};
    }
    if (error) {
        HVN_ERROR_LOG("Couchbase credential read failed: ", error.ec().message());
        throw AuthenticationError{AuthenticationErrorCode::persistence,
                                  "Account could not be loaded"};
    }
    try {
        const auto document = result.content_as<tao::json::value>();
        const auto salt = hex_decode(document.at("passwordSalt").get_string());
        const auto expected_hash = hex_decode(document.at("passwordHash").get_string());
        const auto actual_hash = password_hash(password, salt.data(), salt.size());
        if (expected_hash.size() != actual_hash.size() ||
            CRYPTO_memcmp(expected_hash.data(), actual_hash.data(), actual_hash.size()) != 0) {
            throw AuthenticationError{AuthenticationErrorCode::invalid_credentials,
                                      "Email or password is incorrect"};
        }
        return create_session(connection_, account_from_document(document));
    } catch (const AuthenticationError&) {
        throw;
    } catch (const std::exception& error_detail) {
        HVN_ERROR_LOG("Stored Couchbase credential is invalid: ", error_detail.what());
        throw AuthenticationError{AuthenticationErrorCode::persistence,
                                  "Stored account is invalid"};
    }
}

AuthenticatedAccount CouchbaseAuthenticationService::authenticate(
    const std::string_view access_token) const {
    if (!access_token.starts_with("hvn_") || access_token.size() != 68U) {
        throw AuthenticationError{AuthenticationErrorCode::invalid_session,
                                  "Authentication session is invalid"};
    }
    auto [error, result] =
        connection_->collection(kSessionsCollection).get(session_key(access_token)).get();
    if (error.ec() == ::couchbase::errc::key_value::document_not_found) {
        throw AuthenticationError{AuthenticationErrorCode::invalid_session,
                                  "Authentication session is invalid"};
    }
    if (error) {
        throw AuthenticationError{AuthenticationErrorCode::persistence,
                                  "Authentication session could not be loaded"};
    }
    try {
        const auto document = result.content_as<tao::json::value>();
        if (document.at("expiresAt").get_unsigned() <= now_epoch_seconds()) {
            throw AuthenticationError{AuthenticationErrorCode::invalid_session,
                                      "Authentication session has expired"};
        }
        auto account = account_from_document(document);
        account.access_token = std::string{access_token};
        return account;
    } catch (const AuthenticationError&) {
        throw;
    } catch (const std::exception& error_detail) {
        HVN_ERROR_LOG("Stored Couchbase session is invalid: ", error_detail.what());
        throw AuthenticationError{AuthenticationErrorCode::persistence,
                                  "Stored authentication session is invalid"};
    }
}

void CouchbaseAuthenticationService::logout(const std::string_view access_token) const {
    if (!access_token.starts_with("hvn_") || access_token.size() != 68U) return;
    auto [error, result] =
        connection_->collection(kSessionsCollection).remove(session_key(access_token)).get();
    static_cast<void>(result);
    if (error && error.ec() != ::couchbase::errc::key_value::document_not_found) {
        HVN_WARN_LOG("Couchbase authentication session removal failed: ", error.ec().message());
    }
}

}  // namespace haven::infrastructure::persistence::couchbase

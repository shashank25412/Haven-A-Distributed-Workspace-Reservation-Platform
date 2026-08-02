#include "haven/infrastructure/persistence/couchbase/idempotency_document_key.hpp"

#include "haven/infrastructure/persistence/couchbase/idempotency_document_validator.hpp"

#include <array>
#include <cstdint>
#include <iomanip>
#include <openssl/evp.h>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace haven::infrastructure::persistence::couchbase {
namespace {

void append_field(std::string& encoded, const std::string_view value) {
    const auto size = static_cast<std::uint64_t>(value.size());
    for (int shift = 56; shift >= 0; shift -= 8) {
        encoded.push_back(static_cast<char>((size >> shift) & 0xffU));
    }
    encoded.append(value);
}

std::string sha256_hex(const std::string_view value) {
    auto context = EVP_MD_CTX_new();
    if (context == nullptr) {
        throw std::runtime_error("Unable to allocate SHA-256 context");
    }
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int length{};
    const bool ok = EVP_DigestInit_ex(context, EVP_sha256(), nullptr) == 1 &&
                    EVP_DigestUpdate(context, value.data(), value.size()) == 1 &&
                    EVP_DigestFinal_ex(context, digest.data(), &length) == 1;
    EVP_MD_CTX_free(context);
    if (!ok) {
        throw std::runtime_error("Unable to hash idempotency scope");
    }
    std::ostringstream result;
    result << std::hex << std::setfill('0');
    for (unsigned int index = 0; index < length; ++index) {
        result << std::setw(2) << static_cast<unsigned int>(digest[index]);
    }
    return result.str();
}

}  // namespace

std::string idempotency_document_key(
    const haven::application::idempotency::IdempotencyScope& scope) {
    std::string canonical;
    append_field(canonical, "HAVEN_IDEMPOTENCY_SCOPE_V1");
    append_field(canonical, scope.organization_id().value());
    append_field(canonical, scope.creator_id().value());
    append_field(canonical, idempotency_operation_to_string(scope.operation()));
    append_field(canonical, scope.key().value());
    return "idem::" + sha256_hex(canonical);
}

}  // namespace haven::infrastructure::persistence::couchbase

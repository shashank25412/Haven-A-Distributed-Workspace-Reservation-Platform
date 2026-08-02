/**
 * @file create_reservation_fingerprint.cpp
 * @brief Implements canonical reservation-create request fingerprinting.
 */

#include "haven/application/idempotency/create_reservation_fingerprint_input.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <memory>
#include <openssl/evp.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace haven::application::idempotency {
namespace {

constexpr std::string_view kEncodingIdentifier{"HAVEN_CREATE_RESERVATION_FINGERPRINT_V1"};

void append_unsigned_64(std::string& encoded, const std::uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8) {
        encoded.push_back(static_cast<char>((value >> shift) & 0xffU));
    }
}

void append_string(std::string& encoded, const std::string_view value) {
    append_unsigned_64(encoded, static_cast<std::uint64_t>(value.size()));
    encoded.append(value);
}

void append_timestamp(std::string& encoded, const std::chrono::system_clock::time_point timestamp) {
    const auto nanoseconds =
        std::chrono::duration_cast<std::chrono::nanoseconds>(timestamp.time_since_epoch()).count();
    using Count = decltype(nanoseconds);
    static_assert(sizeof(Count) == sizeof(std::uint64_t));
    append_unsigned_64(encoded, static_cast<std::make_unsigned_t<Count>>(nanoseconds));
}

[[nodiscard]] std::string canonical_encoding(const CreateReservationFingerprintInput& input) {
    auto encoded = std::string{};
    append_string(encoded, kEncodingIdentifier);
    append_string(encoded, input.resource_id().value());
    append_string(encoded, input.creator_id().value());
    append_timestamp(encoded, input.interval().start());
    append_timestamp(encoded, input.interval().end());
    append_string(encoded, input.purpose().value());
    append_string(encoded, haven::domain::to_string(input.reservation_kind()));
    encoded.push_back(input.maintenance_authorized() ? '\x01' : '\x00');
    return encoded;
}

[[nodiscard]] std::string sha256_hex(const std::string_view encoded) {
    using Context = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
    auto context = Context{EVP_MD_CTX_new(), &EVP_MD_CTX_free};
    if (!context || EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) != 1 ||
        EVP_DigestUpdate(context.get(), encoded.data(), encoded.size()) != 1) {
        throw std::runtime_error("Failed to initialize SHA-256 fingerprint calculation");
    }

    auto digest = std::array<unsigned char, EVP_MAX_MD_SIZE>{};
    unsigned int digest_size{};
    if (EVP_DigestFinal_ex(context.get(), digest.data(), &digest_size) != 1 || digest_size != 32U) {
        throw std::runtime_error("Failed to finalize SHA-256 fingerprint calculation");
    }

    auto hexadecimal = std::ostringstream{};
    hexadecimal << std::hex << std::setfill('0');
    for (std::size_t index = 0; index < digest_size; ++index) {
        hexadecimal << std::setw(2) << static_cast<unsigned int>(digest[index]);
    }
    return hexadecimal.str();
}

}  // namespace

IdempotencyFingerprint create_reservation_fingerprint(
    const CreateReservationFingerprintInput& input) {
    return IdempotencyFingerprint{sha256_hex(canonical_encoding(input))};
}

}  // namespace haven::application::idempotency

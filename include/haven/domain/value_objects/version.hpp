/**
 * @file version.hpp
 * @brief Defines the optimistic concurrency version domain value object.
 */

#pragma once

#include <compare>
#include <cstdint>

namespace haven::domain {

/**
 * @brief Represents a persistence-neutral optimistic concurrency version.
 *
 * Version isolates domain and application code from datastore-specific version
 * types such as Couchbase CAS values. Valid aggregates begin at one and advance
 * once for each successful business state change.
 */
class Version final {
public:
    /**
     * @brief Constructs an optimistic concurrency version.
     *
     * @param value Non-negative version value.
     */
    explicit constexpr Version(std::uint64_t value) noexcept : value_(value) {}

    /**
     * @brief Returns the stored version value.
     *
     * @return Persistence-neutral version value.
     */
    [[nodiscard]] constexpr std::uint64_t value() const noexcept {
        return value_;
    }

    constexpr auto operator<=>(const Version&) const = default;

private:
    std::uint64_t value_;
};

}  // namespace haven::domain

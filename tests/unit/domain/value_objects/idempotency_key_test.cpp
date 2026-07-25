/**
 * @file idempotency_key_test.cpp
 * @brief Tests the idempotency key domain value object.
 */

#include "haven/domain/value_objects/idempotency_key.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

namespace haven::domain {
namespace {

TEST(IdempotencyKeyTest, Constructor_ShouldStoreValue_WhenKeyIsValid) {
    const IdempotencyKey idempotency_key{"client-key-123"};

    EXPECT_EQ(idempotency_key.value(), "client-key-123");
}

TEST(IdempotencyKeyTest, Constructor_ShouldPreserveValue_WhenKeyContainsMixedCharacters) {
    const IdempotencyKey idempotency_key{"Client-Key_123.abc"};

    EXPECT_EQ(idempotency_key.value(), "Client-Key_123.abc");
}

TEST(IdempotencyKeyTest, Constructor_ShouldThrow_WhenKeyIsEmpty) {
    EXPECT_THROW(IdempotencyKey{""}, std::invalid_argument);
}

TEST(IdempotencyKeyTest, Equality_ShouldReturnTrue_WhenValuesMatch) {
    const IdempotencyKey first{"client-key-123"};
    const IdempotencyKey second{"client-key-123"};

    EXPECT_EQ(first, second);
}

TEST(IdempotencyKeyTest, Equality_ShouldReturnFalse_WhenValuesDiffer) {
    const IdempotencyKey first{"client-key-123"};
    const IdempotencyKey second{"client-key-456"};

    EXPECT_NE(first, second);
}

}  // namespace
}  // namespace haven::domain
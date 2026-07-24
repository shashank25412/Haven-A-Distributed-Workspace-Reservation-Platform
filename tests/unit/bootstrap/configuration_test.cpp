/**
 * @file configuration_test.cpp
 * @brief Verifies Haven's environment-based bootstrap configuration.
 *
 * These tests temporarily modify process environment variables and restore
 * their original values after each test. They require no external services or
 * network resources.
 */

#include "haven/bootstrap/configuration.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace haven::bootstrap {
namespace {

constexpr const char* kHttpAddressVariable = "HVN_HTTP_ADDRESS";
constexpr const char* kHttpPortVariable = "HVN_HTTP_PORT";
constexpr const char* kHttpThreadsVariable = "HVN_HTTP_THREADS";
constexpr const char* kLogLevelVariable = "HVN_LOG_LEVEL";

/**
 * @brief Provides isolated environment-variable handling for configuration tests.
 *
 * Environment variables are process-global state. The fixture captures their
 * original values before each test and restores them afterward.
 *
 * These tests must not be executed concurrently with other tests that modify
 * the same environment variables.
 */
class EnvironmentConfigurationTest : public ::testing::Test {
protected:
    void SetUp() override {
        original_http_address_ = read_environment_variable(kHttpAddressVariable);
        original_http_port_ = read_environment_variable(kHttpPortVariable);
        original_http_threads_ = read_environment_variable(kHttpThreadsVariable);
        original_log_level_ = read_environment_variable(kLogLevelVariable);
    }

    void TearDown() override {
        EXPECT_TRUE(restore_environment_variable(kHttpAddressVariable, original_http_address_));
        EXPECT_TRUE(restore_environment_variable(kHttpPortVariable, original_http_port_));
        EXPECT_TRUE(restore_environment_variable(kHttpThreadsVariable, original_http_threads_));
        EXPECT_TRUE(restore_environment_variable(kLogLevelVariable, original_log_level_));
    }

    /**
     * @brief Removes all Haven configuration variables for a test.
     */
    void unset_all_configuration_variables() {
        ASSERT_TRUE(unset_environment_variable(kHttpAddressVariable));
        ASSERT_TRUE(unset_environment_variable(kHttpPortVariable));
        ASSERT_TRUE(unset_environment_variable(kHttpThreadsVariable));
        ASSERT_TRUE(unset_environment_variable(kLogLevelVariable));
    }

    /**
     * @brief Reads and copies an environment-variable value.
     */
    [[nodiscard]] static std::optional<std::string> read_environment_variable(
        const char* variable_name) {
        const char* value = std::getenv(variable_name);

        if (value == nullptr) {
            return std::nullopt;
        }

        return std::string{value};
    }

    /**
     * @brief Sets an environment variable for the current test process.
     */
    [[nodiscard]] static bool set_environment_variable(
        const char* variable_name,
        const std::string_view value) {
#if defined(_WIN32)
        return _putenv_s(variable_name, std::string{value}.c_str()) == 0;
#else
        return ::setenv(variable_name, std::string{value}.c_str(), 1) == 0;
#endif
    }

    /**
     * @brief Removes an environment variable from the current test process.
     */
    [[nodiscard]] static bool unset_environment_variable(const char* variable_name) {
#if defined(_WIN32)
        return _putenv_s(variable_name, "") == 0;
#else
        return ::unsetenv(variable_name) == 0;
#endif
    }

private:
    /**
     * @brief Restores an environment variable to its pre-test state.
     */
    [[nodiscard]] static bool restore_environment_variable(
        const char* variable_name,
        const std::optional<std::string>& original_value) {
        if (original_value.has_value()) {
            return set_environment_variable(variable_name, original_value.value());
        }

        return unset_environment_variable(variable_name);
    }

    std::optional<std::string> original_http_address_;
    std::optional<std::string> original_http_port_;
    std::optional<std::string> original_http_threads_;
    std::optional<std::string> original_log_level_;
};

TEST_F(EnvironmentConfigurationTest, UsesDocumentedDefaultsWhenVariablesAreAbsent) {
    unset_all_configuration_variables();

    const ApplicationConfiguration configuration = load_configuration_from_environment();

    EXPECT_EQ(configuration.http.address, "0.0.0.0");
    EXPECT_EQ(configuration.http.port, std::uint16_t{8080});
    EXPECT_EQ(configuration.http.worker_threads, std::uint32_t{1});
    EXPECT_EQ(configuration.logging.level, LogLevel::info);
}

TEST_F(EnvironmentConfigurationTest, LoadsConfiguredValues) {
    ASSERT_TRUE(set_environment_variable(kHttpAddressVariable, "127.0.0.1"));
    ASSERT_TRUE(set_environment_variable(kHttpPortVariable, "9090"));
    ASSERT_TRUE(set_environment_variable(kHttpThreadsVariable, "4"));
    ASSERT_TRUE(set_environment_variable(kLogLevelVariable, "debug"));

    const ApplicationConfiguration configuration = load_configuration_from_environment();

    EXPECT_EQ(configuration.http.address, "127.0.0.1");
    EXPECT_EQ(configuration.http.port, std::uint16_t{9090});
    EXPECT_EQ(configuration.http.worker_threads, std::uint32_t{4});
    EXPECT_EQ(configuration.logging.level, LogLevel::debug);
}

TEST_F(EnvironmentConfigurationTest, UsesDefaultsWhenVariablesArePresentButEmpty) {
    ASSERT_TRUE(set_environment_variable(kHttpAddressVariable, ""));
    ASSERT_TRUE(set_environment_variable(kHttpPortVariable, ""));
    ASSERT_TRUE(set_environment_variable(kHttpThreadsVariable, ""));
    ASSERT_TRUE(set_environment_variable(kLogLevelVariable, ""));

    const ApplicationConfiguration configuration = load_configuration_from_environment();

    EXPECT_EQ(configuration.http.address, "0.0.0.0");
    EXPECT_EQ(configuration.http.port, std::uint16_t{8080});
    EXPECT_EQ(configuration.http.worker_threads, std::uint32_t{1});
    EXPECT_EQ(configuration.logging.level, LogLevel::info);
}

TEST_F(EnvironmentConfigurationTest, RejectsPortValuesOutsideTheValidTcpRange) {
    ASSERT_TRUE(set_environment_variable(kHttpAddressVariable, "0.0.0.0"));

    constexpr std::array<std::string_view, 6> kInvalidPorts{
        "0",
        "65536",
        "-1",
        "8080abc",
        "abc",
        "80.80",
    };

    for (const std::string_view invalid_port : kInvalidPorts) {
        SCOPED_TRACE(invalid_port);

        ASSERT_TRUE(set_environment_variable(kHttpPortVariable, invalid_port));
        EXPECT_THROW( static_cast<void>(load_configuration_from_environment()), ConfigurationError);
    }
}

TEST_F(EnvironmentConfigurationTest, AcceptsTcpPortBoundaryValues) {
    ASSERT_TRUE(set_environment_variable(kHttpPortVariable, "1"));

    ApplicationConfiguration configuration = load_configuration_from_environment();

    EXPECT_EQ(configuration.http.port, std::uint16_t{1});

    ASSERT_TRUE(set_environment_variable(kHttpPortVariable, "65535"));

    configuration = load_configuration_from_environment();

    EXPECT_EQ(configuration.http.port, std::uint16_t{65535});
}

TEST_F(EnvironmentConfigurationTest, RejectsInvalidHttpWorkerThreadCounts) {
    constexpr std::array<std::string_view, 6> kInvalidThreadCounts{
        "0",
        "-1",
        "4abc",
        "abc",
        "2.5",
        "4294967296",
    };

    for (const std::string_view invalid_thread_count : kInvalidThreadCounts) {
        SCOPED_TRACE(invalid_thread_count);

        ASSERT_TRUE(set_environment_variable(kHttpThreadsVariable, invalid_thread_count));
        EXPECT_THROW( static_cast<void>(load_configuration_from_environment()), ConfigurationError);
    }
}

TEST_F(EnvironmentConfigurationTest, AcceptsPositiveHttpWorkerThreadCount) {
    ASSERT_TRUE(set_environment_variable(kHttpThreadsVariable, "8"));

    const ApplicationConfiguration configuration = load_configuration_from_environment();

    EXPECT_EQ(configuration.http.worker_threads, std::uint32_t{8});
}

TEST_F(EnvironmentConfigurationTest, ParsesSupportedLogLevelsCaseInsensitively) {
    const std::array<std::pair<std::string_view, LogLevel>, 7> kSupportedLevels{
        std::pair{"trace", LogLevel::trace},
        std::pair{"DEBUG", LogLevel::debug},
        std::pair{"Info", LogLevel::info},
        std::pair{"warn", LogLevel::warn},
        std::pair{"WARNING", LogLevel::warn},
        std::pair{"error", LogLevel::error},
        std::pair{"Critical", LogLevel::critical},
    };

    for (const auto& [configured_value, expected_level] : kSupportedLevels) {
        SCOPED_TRACE(configured_value);

        ASSERT_TRUE(set_environment_variable(kLogLevelVariable, configured_value));

        const ApplicationConfiguration configuration = load_configuration_from_environment();

        EXPECT_EQ(configuration.logging.level, expected_level);
    }
}

TEST_F(EnvironmentConfigurationTest, RejectsUnsupportedLogLevels) {
    constexpr std::array<std::string_view, 5> kInvalidLogLevels{
        "verbose",
        "fatal",
        "notice",
        "123",
        " info ",
    };

    for (const std::string_view invalid_log_level : kInvalidLogLevels) {
        SCOPED_TRACE(invalid_log_level);

        ASSERT_TRUE(set_environment_variable(kLogLevelVariable, invalid_log_level));
        EXPECT_THROW( static_cast<void>(load_configuration_from_environment()), ConfigurationError);
    }
}

TEST_F(EnvironmentConfigurationTest, RejectsWhitespaceOnlyHttpAddress) {
    ASSERT_TRUE(set_environment_variable(kHttpAddressVariable, "   \t"));

    EXPECT_THROW( static_cast<void>(load_configuration_from_environment()), ConfigurationError);
}

}  // namespace
}  // namespace haven::bootstrap

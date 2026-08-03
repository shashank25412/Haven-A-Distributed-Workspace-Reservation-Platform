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
constexpr const char* kMetricsEnabledVariable = "HVN_METRICS_ENABLED";
constexpr const char* kCouchbaseConnectionStringVariable = "HVN_COUCHBASE_CONNECTION_STRING";
constexpr const char* kCouchbaseUsernameVariable = "HVN_COUCHBASE_USERNAME";
constexpr const char* kCouchbasePasswordVariable = "HVN_COUCHBASE_PASSWORD";
constexpr const char* kCouchbaseBucketVariable = "HVN_COUCHBASE_BUCKET";
constexpr const char* kCouchbaseScopeVariable = "HVN_COUCHBASE_SCOPE";
constexpr const char* kIdempotencyRetentionVariable = "HVN_IDEMPOTENCY_RETENTION_SECONDS";
constexpr const char* kRedisEnabledVariable = "HVN_REDIS_ENABLED";
constexpr const char* kRedisUriVariable = "HVN_REDIS_URI";
constexpr const char* kRedisConnectTimeoutVariable = "HVN_REDIS_CONNECT_TIMEOUT_MS";
constexpr const char* kRedisCommandTimeoutVariable = "HVN_REDIS_COMMAND_TIMEOUT_MS";
constexpr const char* kRedisResourceTtlVariable = "HVN_REDIS_RESOURCE_TTL_SECONDS";
constexpr const char* kKafkaEnabledVariable = "HVN_KAFKA_ENABLED";
constexpr const char* kKafkaBrokersVariable = "HVN_KAFKA_BROKERS";
constexpr const char* kKafkaTopicVariable = "HVN_KAFKA_RESERVATION_EVENTS_TOPIC";
constexpr const char* kKafkaClientIdVariable = "HVN_KAFKA_CLIENT_ID";
constexpr const char* kKafkaAckTimeoutVariable = "HVN_KAFKA_ACK_TIMEOUT_MS";
constexpr const char* kKafkaDeliveryTimeoutVariable = "HVN_KAFKA_DELIVERY_TIMEOUT_MS";
constexpr const char* kOutboxPublisherBatchSizeVariable = "HVN_OUTBOX_PUBLISHER_BATCH_SIZE";
constexpr const char* kOutboxPublisherPollIntervalVariable =
    "HVN_OUTBOX_PUBLISHER_POLL_INTERVAL_MS";
constexpr std::string_view kTestPassword{"test-secret-password"};

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
        original_metrics_enabled_ = read_environment_variable(kMetricsEnabledVariable);
        original_couchbase_connection_string_ =
            read_environment_variable(kCouchbaseConnectionStringVariable);
        original_couchbase_username_ = read_environment_variable(kCouchbaseUsernameVariable);
        original_couchbase_password_ = read_environment_variable(kCouchbasePasswordVariable);
        original_couchbase_bucket_ = read_environment_variable(kCouchbaseBucketVariable);
        original_couchbase_scope_ = read_environment_variable(kCouchbaseScopeVariable);
        original_idempotency_retention_ = read_environment_variable(kIdempotencyRetentionVariable);
        original_redis_enabled_ = read_environment_variable(kRedisEnabledVariable);
        original_redis_uri_ = read_environment_variable(kRedisUriVariable);
        original_redis_connect_timeout_ = read_environment_variable(kRedisConnectTimeoutVariable);
        original_redis_command_timeout_ = read_environment_variable(kRedisCommandTimeoutVariable);
        original_redis_resource_ttl_ = read_environment_variable(kRedisResourceTtlVariable);
        const std::array kafka_variables{kKafkaEnabledVariable,
                                         kKafkaBrokersVariable,
                                         kKafkaTopicVariable,
                                         kKafkaClientIdVariable,
                                         kKafkaAckTimeoutVariable,
                                         kKafkaDeliveryTimeoutVariable,
                                         kOutboxPublisherBatchSizeVariable,
                                         kOutboxPublisherPollIntervalVariable};
        for (std::size_t index = 0; index < kafka_variables.size(); ++index) {
            original_kafka_[index] = read_environment_variable(kafka_variables[index]);
            ASSERT_TRUE(unset_environment_variable(kafka_variables[index]));
        }

        set_valid_couchbase_configuration();
        ASSERT_TRUE(unset_environment_variable(kMetricsEnabledVariable));
        ASSERT_TRUE(unset_environment_variable(kRedisEnabledVariable));
        ASSERT_TRUE(unset_environment_variable(kRedisUriVariable));
        ASSERT_TRUE(unset_environment_variable(kRedisConnectTimeoutVariable));
        ASSERT_TRUE(unset_environment_variable(kRedisCommandTimeoutVariable));
        ASSERT_TRUE(unset_environment_variable(kRedisResourceTtlVariable));
        ASSERT_TRUE(unset_environment_variable(kIdempotencyRetentionVariable));
    }

    void TearDown() override {
        EXPECT_TRUE(restore_environment_variable(kHttpAddressVariable, original_http_address_));
        EXPECT_TRUE(restore_environment_variable(kHttpPortVariable, original_http_port_));
        EXPECT_TRUE(restore_environment_variable(kHttpThreadsVariable, original_http_threads_));
        EXPECT_TRUE(restore_environment_variable(kLogLevelVariable, original_log_level_));
        EXPECT_TRUE(
            restore_environment_variable(kMetricsEnabledVariable, original_metrics_enabled_));
        EXPECT_TRUE(restore_environment_variable(kCouchbaseConnectionStringVariable,
                                                 original_couchbase_connection_string_));
        EXPECT_TRUE(
            restore_environment_variable(kCouchbaseUsernameVariable, original_couchbase_username_));
        EXPECT_TRUE(
            restore_environment_variable(kCouchbasePasswordVariable, original_couchbase_password_));
        EXPECT_TRUE(
            restore_environment_variable(kCouchbaseBucketVariable, original_couchbase_bucket_));
        EXPECT_TRUE(
            restore_environment_variable(kCouchbaseScopeVariable, original_couchbase_scope_));
        EXPECT_TRUE(restore_environment_variable(kIdempotencyRetentionVariable,
                                                 original_idempotency_retention_));
        EXPECT_TRUE(restore_environment_variable(kRedisEnabledVariable, original_redis_enabled_));
        EXPECT_TRUE(restore_environment_variable(kRedisUriVariable, original_redis_uri_));
        EXPECT_TRUE(restore_environment_variable(kRedisConnectTimeoutVariable,
                                                 original_redis_connect_timeout_));
        EXPECT_TRUE(restore_environment_variable(kRedisCommandTimeoutVariable,
                                                 original_redis_command_timeout_));
        EXPECT_TRUE(
            restore_environment_variable(kRedisResourceTtlVariable, original_redis_resource_ttl_));
        const std::array kafka_variables{kKafkaEnabledVariable,
                                         kKafkaBrokersVariable,
                                         kKafkaTopicVariable,
                                         kKafkaClientIdVariable,
                                         kKafkaAckTimeoutVariable,
                                         kKafkaDeliveryTimeoutVariable,
                                         kOutboxPublisherBatchSizeVariable,
                                         kOutboxPublisherPollIntervalVariable};
        for (std::size_t index = 0; index < kafka_variables.size(); ++index)
            EXPECT_TRUE(
                restore_environment_variable(kafka_variables[index], original_kafka_[index]));
    }

    /**
     * @brief Removes all Haven configuration variables for a test.
     */
    void unset_all_configuration_variables() {
        ASSERT_TRUE(unset_environment_variable(kHttpAddressVariable));
        ASSERT_TRUE(unset_environment_variable(kHttpPortVariable));
        ASSERT_TRUE(unset_environment_variable(kHttpThreadsVariable));
        ASSERT_TRUE(unset_environment_variable(kLogLevelVariable));
        ASSERT_TRUE(unset_environment_variable(kMetricsEnabledVariable));
    }

    /**
     * @brief Sets deterministic valid Couchbase configuration for a test.
     */
    void set_valid_couchbase_configuration() {
        ASSERT_TRUE(set_environment_variable(kCouchbaseConnectionStringVariable,
                                             "couchbase://127.0.0.1?network=external"));
        ASSERT_TRUE(set_environment_variable(kCouchbaseUsernameVariable, "test-user"));
        ASSERT_TRUE(set_environment_variable(kCouchbasePasswordVariable, kTestPassword));
        ASSERT_TRUE(set_environment_variable(kCouchbaseBucketVariable, "test-bucket"));
        ASSERT_TRUE(set_environment_variable(kCouchbaseScopeVariable, "test-scope"));
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
    [[nodiscard]] static bool set_environment_variable(const char* variable_name,
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
        const char* variable_name, const std::optional<std::string>& original_value) {
        if (original_value.has_value()) {
            return set_environment_variable(variable_name, original_value.value());
        }

        return unset_environment_variable(variable_name);
    }

    std::optional<std::string> original_http_address_;
    std::optional<std::string> original_http_port_;
    std::optional<std::string> original_http_threads_;
    std::optional<std::string> original_log_level_;
    std::optional<std::string> original_metrics_enabled_;
    std::optional<std::string> original_couchbase_connection_string_;
    std::optional<std::string> original_couchbase_username_;
    std::optional<std::string> original_couchbase_password_;
    std::optional<std::string> original_couchbase_bucket_;
    std::optional<std::string> original_couchbase_scope_;
    std::optional<std::string> original_idempotency_retention_;
    std::optional<std::string> original_redis_enabled_;
    std::optional<std::string> original_redis_uri_;
    std::optional<std::string> original_redis_connect_timeout_;
    std::optional<std::string> original_redis_command_timeout_;
    std::optional<std::string> original_redis_resource_ttl_;
    std::array<std::optional<std::string>, 8> original_kafka_;
};

TEST_F(EnvironmentConfigurationTest, EnablesMetricsByDefault) {
    EXPECT_TRUE(load_configuration_from_environment().metrics.enabled);
}

TEST_F(EnvironmentConfigurationTest, ParsesMetricsEnabledUsingBooleanRules) {
    for (const auto value : {"true", "TRUE", "1"}) {
        ASSERT_TRUE(set_environment_variable(kMetricsEnabledVariable, value));
        EXPECT_TRUE(load_configuration_from_environment().metrics.enabled);
    }
    for (const auto value : {"false", "FALSE", "0"}) {
        ASSERT_TRUE(set_environment_variable(kMetricsEnabledVariable, value));
        EXPECT_FALSE(load_configuration_from_environment().metrics.enabled);
    }
}

TEST_F(EnvironmentConfigurationTest, RejectsInvalidMetricsEnabledValue) {
    ASSERT_TRUE(set_environment_variable(kMetricsEnabledVariable, "yes"));
    EXPECT_THROW(static_cast<void>(load_configuration_from_environment()), ConfigurationError);
}

TEST_F(EnvironmentConfigurationTest, LoadsOutboxPublisherRuntimeDefaults) {
    const auto publisher = load_configuration_from_environment().outbox_publisher;
    EXPECT_EQ(publisher.batch_size, 100U);
    EXPECT_EQ(publisher.poll_interval, std::chrono::milliseconds{1000});
}

TEST_F(EnvironmentConfigurationTest, ParsesOutboxPublisherRuntimeConfiguration) {
    ASSERT_TRUE(set_environment_variable(kOutboxPublisherBatchSizeVariable, "27"));
    ASSERT_TRUE(set_environment_variable(kOutboxPublisherPollIntervalVariable, "250"));
    const auto publisher = load_configuration_from_environment().outbox_publisher;
    EXPECT_EQ(publisher.batch_size, 27U);
    EXPECT_EQ(publisher.poll_interval, std::chrono::milliseconds{250});
}

TEST_F(EnvironmentConfigurationTest, RejectsInvalidOutboxPublisherRuntimeConfiguration) {
    for (const auto value : {"0", "-1", "invalid", "18446744073709551616"}) {
        ASSERT_TRUE(set_environment_variable(kOutboxPublisherBatchSizeVariable, value));
        EXPECT_THROW(static_cast<void>(load_configuration_from_environment()), ConfigurationError);
    }
    ASSERT_TRUE(set_environment_variable(kOutboxPublisherBatchSizeVariable, "100"));
    for (const auto value : {"0", "-1", "invalid", "9223372036854775808"}) {
        ASSERT_TRUE(set_environment_variable(kOutboxPublisherPollIntervalVariable, value));
        EXPECT_THROW(static_cast<void>(load_configuration_from_environment()), ConfigurationError);
    }
}

TEST_F(EnvironmentConfigurationTest, LoadsKafkaDisabledDefaults) {
    const auto kafka = load_configuration_from_environment().kafka;
    EXPECT_FALSE(kafka.enabled);
    EXPECT_EQ(kafka.brokers, "127.0.0.1:9092");
    EXPECT_EQ(kafka.reservation_events_topic, "haven.reservation.events");
    EXPECT_EQ(kafka.client_id, "haven-reservation-producer");
}

TEST_F(EnvironmentConfigurationTest, ParsesEnabledKafkaConfiguration) {
    ASSERT_TRUE(set_environment_variable(kKafkaEnabledVariable, "true"));
    ASSERT_TRUE(set_environment_variable(kKafkaBrokersVariable, "kafka:29092"));
    ASSERT_TRUE(set_environment_variable(kKafkaTopicVariable, "events"));
    ASSERT_TRUE(set_environment_variable(kKafkaClientIdVariable, "producer"));
    ASSERT_TRUE(set_environment_variable(kKafkaAckTimeoutVariable, "250"));
    ASSERT_TRUE(set_environment_variable(kKafkaDeliveryTimeoutVariable, "500"));
    const auto kafka = load_configuration_from_environment().kafka;
    EXPECT_TRUE(kafka.enabled);
    EXPECT_EQ(kafka.brokers, "kafka:29092");
    EXPECT_EQ(kafka.acknowledgement_timeout, std::chrono::milliseconds{250});
    EXPECT_EQ(kafka.delivery_timeout, std::chrono::milliseconds{500});
}

TEST_F(EnvironmentConfigurationTest, RejectsInvalidKafkaConfiguration) {
    ASSERT_TRUE(set_environment_variable(kKafkaEnabledVariable, "true"));
    ASSERT_TRUE(set_environment_variable(kKafkaBrokersVariable, "   "));
    EXPECT_THROW(static_cast<void>(load_configuration_from_environment()), ConfigurationError);
    ASSERT_TRUE(set_environment_variable(kKafkaBrokersVariable, "localhost:9092"));
    ASSERT_TRUE(set_environment_variable(kKafkaTopicVariable, "   "));
    EXPECT_THROW(static_cast<void>(load_configuration_from_environment()), ConfigurationError);
    ASSERT_TRUE(set_environment_variable(kKafkaTopicVariable, "events"));
    ASSERT_TRUE(set_environment_variable(kKafkaAckTimeoutVariable, "501"));
    ASSERT_TRUE(set_environment_variable(kKafkaDeliveryTimeoutVariable, "500"));
    EXPECT_THROW(static_cast<void>(load_configuration_from_environment()), ConfigurationError);
    ASSERT_TRUE(set_environment_variable(kKafkaAckTimeoutVariable, "500"));
    ASSERT_TRUE(set_environment_variable(kKafkaClientIdVariable, "   "));
    EXPECT_THROW(static_cast<void>(load_configuration_from_environment()), ConfigurationError);
    ASSERT_TRUE(set_environment_variable(kKafkaClientIdVariable, "producer"));
    ASSERT_TRUE(set_environment_variable(kKafkaAckTimeoutVariable, "0"));
    EXPECT_THROW(static_cast<void>(load_configuration_from_environment()), ConfigurationError);
}

TEST_F(EnvironmentConfigurationTest, DisabledKafkaDoesNotRequireUsableBrokerOrTopic) {
    ASSERT_TRUE(set_environment_variable(kKafkaBrokersVariable, "   "));
    ASSERT_TRUE(set_environment_variable(kKafkaTopicVariable, "   "));
    EXPECT_FALSE(load_configuration_from_environment().kafka.enabled);
}

TEST_F(EnvironmentConfigurationTest, LoadsDocumentedRedisDefaults) {
    const auto configuration = load_configuration_from_environment();
    EXPECT_FALSE(configuration.redis.enabled);
    EXPECT_EQ(configuration.redis.uri, "tcp://127.0.0.1:6379");
    EXPECT_EQ(configuration.redis.connect_timeout, std::chrono::milliseconds{100});
    EXPECT_EQ(configuration.redis.command_timeout, std::chrono::milliseconds{100});
    EXPECT_EQ(configuration.redis.resource_detail_ttl, std::chrono::seconds{300});
}

TEST_F(EnvironmentConfigurationTest, LoadsConfiguredRedisValues) {
    ASSERT_TRUE(set_environment_variable(kRedisEnabledVariable, "true"));
    ASSERT_TRUE(set_environment_variable(kRedisUriVariable, "tcp://redis.local:6380"));
    ASSERT_TRUE(set_environment_variable(kRedisConnectTimeoutVariable, "25"));
    ASSERT_TRUE(set_environment_variable(kRedisCommandTimeoutVariable, "50"));
    ASSERT_TRUE(set_environment_variable(kRedisResourceTtlVariable, "600"));
    const auto configuration = load_configuration_from_environment();
    EXPECT_TRUE(configuration.redis.enabled);
    EXPECT_EQ(configuration.redis.resource_detail_ttl, std::chrono::seconds{600});
}

TEST_F(EnvironmentConfigurationTest, RejectsInvalidRedisConfiguration) {
    ASSERT_TRUE(set_environment_variable(kRedisResourceTtlVariable, "0"));
    EXPECT_THROW(load_configuration_from_environment(), ConfigurationError);
    ASSERT_TRUE(set_environment_variable(kRedisResourceTtlVariable, "300"));
    ASSERT_TRUE(set_environment_variable(kRedisConnectTimeoutVariable, "invalid"));
    EXPECT_THROW(load_configuration_from_environment(), ConfigurationError);
    ASSERT_TRUE(set_environment_variable(kRedisConnectTimeoutVariable, "100"));
    ASSERT_TRUE(set_environment_variable(kRedisUriVariable, "not-a-redis-uri"));
    EXPECT_THROW(load_configuration_from_environment(), ConfigurationError);
}

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

TEST_F(EnvironmentConfigurationTest, LoadsAllCouchbaseEnvironmentVariablesExactly) {
    const ApplicationConfiguration configuration = load_configuration_from_environment();

    EXPECT_EQ(configuration.couchbase.connection_string, "couchbase://127.0.0.1?network=external");
    EXPECT_EQ(configuration.couchbase.username, "test-user");
    EXPECT_EQ(configuration.couchbase.password, kTestPassword);
    EXPECT_EQ(configuration.couchbase.bucket_name, "test-bucket");
    EXPECT_EQ(configuration.couchbase.scope_name, "test-scope");
    EXPECT_EQ(configuration.couchbase.idempotency_retention, std::chrono::seconds{86400});
}

TEST_F(EnvironmentConfigurationTest, LoadsConfiguredIdempotencyRetention) {
    ASSERT_TRUE(set_environment_variable(kIdempotencyRetentionVariable, "90"));
    EXPECT_EQ(load_configuration_from_environment().couchbase.idempotency_retention,
              std::chrono::seconds{90});
}

TEST_F(EnvironmentConfigurationTest, RejectsInvalidIdempotencyRetention) {
    for (const auto value : {"0", "-1", "invalid", "9223372036854775808", "18446744073709551616"}) {
        ASSERT_TRUE(set_environment_variable(kIdempotencyRetentionVariable, value));
        EXPECT_THROW(static_cast<void>(load_configuration_from_environment()), ConfigurationError);
    }
}

TEST_F(EnvironmentConfigurationTest, RejectsMissingCouchbaseEnvironmentVariables) {
    constexpr std::array<const char*, 5> kRequiredVariables{
        kCouchbaseConnectionStringVariable,
        kCouchbaseUsernameVariable,
        kCouchbasePasswordVariable,
        kCouchbaseBucketVariable,
        kCouchbaseScopeVariable,
    };

    for (const char* variable : kRequiredVariables) {
        SCOPED_TRACE(variable);
        set_valid_couchbase_configuration();
        ASSERT_TRUE(unset_environment_variable(variable));

        try {
            static_cast<void>(load_configuration_from_environment());
            FAIL() << "Expected ConfigurationError for " << variable;
        } catch (const ConfigurationError& error) {
            EXPECT_NE(std::string{error.what()}.find(variable), std::string::npos);
            EXPECT_EQ(std::string{error.what()}.find(kTestPassword), std::string::npos);
        }
    }
}

TEST_F(EnvironmentConfigurationTest, RejectsEmptyCouchbaseEnvironmentVariables) {
    constexpr std::array<const char*, 5> kRequiredVariables{
        kCouchbaseConnectionStringVariable,
        kCouchbaseUsernameVariable,
        kCouchbasePasswordVariable,
        kCouchbaseBucketVariable,
        kCouchbaseScopeVariable,
    };

    for (const char* variable : kRequiredVariables) {
        SCOPED_TRACE(variable);
        set_valid_couchbase_configuration();
        ASSERT_TRUE(set_environment_variable(variable, ""));

        try {
            static_cast<void>(load_configuration_from_environment());
            FAIL() << "Expected ConfigurationError for " << variable;
        } catch (const ConfigurationError& error) {
            EXPECT_NE(std::string{error.what()}.find(variable), std::string::npos);
            EXPECT_EQ(std::string{error.what()}.find(kTestPassword), std::string::npos);
        }
    }
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
        EXPECT_THROW(static_cast<void>(load_configuration_from_environment()), ConfigurationError);
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
        EXPECT_THROW(static_cast<void>(load_configuration_from_environment()), ConfigurationError);
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
        EXPECT_THROW(static_cast<void>(load_configuration_from_environment()), ConfigurationError);
    }
}

TEST_F(EnvironmentConfigurationTest, RejectsWhitespaceOnlyHttpAddress) {
    ASSERT_TRUE(set_environment_variable(kHttpAddressVariable, "   \t"));

    EXPECT_THROW(static_cast<void>(load_configuration_from_environment()), ConfigurationError);
}

}  // namespace
}  // namespace haven::bootstrap

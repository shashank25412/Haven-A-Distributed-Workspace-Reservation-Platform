/**
 * @file logging_test.cpp
 * @brief Verifies log filtering, formatting, output redirection, and scope tracing.
 */

#include "haven/logging/logging.hpp"

#include <gtest/gtest.h>

#include <iostream>
#include <sstream>
#include <string>

namespace haven::logging {
namespace {

class LoggingTest : public testing::Test {
protected:
    void SetUp() override {
        Logger::instance().set_output(output_);
        Logger::instance().set_level(LogLevel::Trace);
    }

    void TearDown() override {
        Logger::instance().set_output(std::clog);
        Logger::instance().set_level(LogLevel::Info);
    }

    std::ostringstream output_;
};

void traced_function() {
    HVN_TRACE_SCOPE();
}

TEST_F(LoggingTest, WritesMessageWithConfiguredLevel) {
    HVN_INFO_LOG("reservation_id=", 42, " status=confirmed");

    EXPECT_EQ(output_.str(), "[INFO] reservation_id=42 status=confirmed\n");
}

TEST_F(LoggingTest, FiltersMessagesBelowConfiguredLevel) {
    Logger::instance().set_level(LogLevel::Warn);

    HVN_TRACE_LOG("trace message");
    HVN_DEBUG_LOG("debug message");
    HVN_INFO_LOG("info message");
    HVN_WARN_LOG("warning message");

    EXPECT_EQ(output_.str(), "[WARN] warning message\n");
}

TEST_F(LoggingTest, DoesNotWriteMessagesWhenLoggingIsDisabled) {
    Logger::instance().set_level(LogLevel::Off);

    HVN_CRITICAL_LOG("critical message");

    EXPECT_TRUE(output_.str().empty());
}

TEST_F(LoggingTest, SupportsMultipleMessageArguments) {
    const std::string resource_id = "meeting-room-101";
    const int organization_id = 12;

    HVN_DEBUG_LOG("resource_id=", resource_id, " organization_id=", organization_id);

    EXPECT_EQ(output_.str(), "[DEBUG] resource_id=meeting-room-101 organization_id=12\n");
}

TEST_F(LoggingTest, TracesFunctionEntryAndExit) {
    traced_function();

    const std::string log_output = output_.str();

    EXPECT_NE(log_output.find("[TRACE] ==> logging_test.cpp::traced_function\n"), std::string::npos);
    EXPECT_NE(log_output.find("[TRACE] <== logging_test.cpp::traced_function duration_us="), std::string::npos);
}

TEST(LogLevelTest, ConvertsLevelsToText) {
    EXPECT_EQ(to_string(LogLevel::Trace), "TRACE");
    EXPECT_EQ(to_string(LogLevel::Debug), "DEBUG");
    EXPECT_EQ(to_string(LogLevel::Info), "INFO");
    EXPECT_EQ(to_string(LogLevel::Warn), "WARN");
    EXPECT_EQ(to_string(LogLevel::Error), "ERROR");
    EXPECT_EQ(to_string(LogLevel::Critical), "CRITICAL");
    EXPECT_EQ(to_string(LogLevel::Off), "OFF");
}

}  // namespace
}  // namespace haven::logging
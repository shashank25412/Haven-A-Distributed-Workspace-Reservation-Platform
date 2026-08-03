/**
 * @file disabled_main.cpp
 * @brief Runs HTTP tests with the metrics route disabled.
 */

#define DROGON_TEST_MAIN
#include <drogon/HttpAppFramework.h>
#include <drogon/drogon_test.h>

#include <cstdint>
#include <future>
#include <thread>

int main(int argc, char** argv) {
    drogon::app().addListener("127.0.0.1", std::uint16_t{18084}).setThreadNum(1);
    std::promise<void> started;
    auto started_future = started.get_future();
    std::thread server([&started]() {
        drogon::app().getLoop()->queueInLoop([&started]() { started.set_value(); });
        drogon::app().run();
    });
    started_future.get();
    const int result = drogon::test::run(argc, argv);
    drogon::app().getLoop()->queueInLoop([]() { drogon::app().quit(); });
    server.join();
    return result;
}

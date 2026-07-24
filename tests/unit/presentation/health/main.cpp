/**
 * @file live_controller_test_main.cpp
 * @brief Defines the test runner for Haven's liveness controller tests.
 *
 * This runner registers the liveness route, starts the Drogon event loop,
 * executes the asynchronous controller tests, and shuts Drogon down cleanly.
 */

#define DROGON_TEST_MAIN
#include <drogon/drogon_test.h>

#include "haven/presentation/health/live_controller.hpp"

#include <drogon/HttpAppFramework.h>

#include <future>
#include <thread>

namespace {

constexpr char kTestServerAddress[] = "127.0.0.1";
constexpr std::uint16_t kTestServerPort = 18080;
constexpr std::size_t kTestWorkerThreads = 1;

}  // namespace

int main(int argc, char** argv) {
    haven::presentation::health::register_live_route();

    drogon::app()
        .addListener(kTestServerAddress, kTestServerPort)
        .setThreadNum(kTestWorkerThreads);

    std::promise<void> event_loop_started;
    std::future<void> event_loop_started_future = event_loop_started.get_future();

    std::thread server_thread([&event_loop_started]() {
        drogon::app().getLoop()->queueInLoop(
            [&event_loop_started]() { event_loop_started.set_value(); });

        drogon::app().run();
    });

    event_loop_started_future.get();

    const int test_result = drogon::test::run(argc, argv);

    drogon::app().getLoop()->queueInLoop([]() { drogon::app().quit(); });
    server_thread.join();

    return test_result;
}
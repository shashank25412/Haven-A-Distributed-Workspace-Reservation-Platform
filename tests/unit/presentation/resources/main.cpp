/**
 * @file resource_controller_test_main.cpp
 * @brief Runs Resource controller tests against a local Drogon listener.
 */

#define DROGON_TEST_MAIN
#include "haven/application/resources/authoritative_resource_query_repository.hpp"
#include "haven/application/resources/get_resource_handler.hpp"
#include "haven/presentation/resources/get_resource_controller.hpp"

#include "resources/test_resource_repository.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/drogon_test.h>

#include <cstdint>
#include <future>
#include <memory>
#include <thread>

namespace haven::presentation::resources::test {

TestResourceRepository repository;

}  // namespace haven::presentation::resources::test

namespace {

constexpr char kTestServerAddress[] = "127.0.0.1";
constexpr std::uint16_t kTestServerPort = 18081;
constexpr std::size_t kTestWorkerThreads = 1;

}  // namespace

int main(int argc, char** argv) {
    auto handler = [&]() {
        static haven::application::resources::AuthoritativeResourceQueryRepository query{
            haven::presentation::resources::test::repository};
        return std::make_shared<haven::application::resources::GetResourceHandler>(query);
    }();
    haven::presentation::resources::register_get_resource_route(std::move(handler));

    drogon::app().addListener(kTestServerAddress, kTestServerPort).setThreadNum(kTestWorkerThreads);

    std::promise<void> event_loop_started;
    auto event_loop_started_future = event_loop_started.get_future();
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

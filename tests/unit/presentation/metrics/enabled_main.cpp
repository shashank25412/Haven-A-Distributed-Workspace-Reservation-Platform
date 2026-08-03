/**
 * @file enabled_main.cpp
 * @brief Runs HTTP tests with the metrics route enabled.
 */

#define DROGON_TEST_MAIN
#include "haven/application/observability/metrics/metric_label.hpp"
#include "haven/application/observability/metrics/metric_name.hpp"
#include "haven/infrastructure/observability/metrics/prometheus_metrics_recorder.hpp"
#include "haven/presentation/observability/metrics/metrics_controller.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/drogon_test.h>

#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <thread>

int main(int argc, char** argv) {
    using haven::application::observability::metrics::MetricLabel;
    using haven::application::observability::metrics::MetricName;
    using haven::infrastructure::observability::metrics::PrometheusMetricsRecorder;
    using namespace std::chrono_literals;

    auto recorder = std::make_shared<PrometheusMetricsRecorder>();
    recorder->increment_counter(
        MetricName{"haven_test_requests_total"}, 3.0, {MetricLabel{"result", "ok"}});
    recorder->set_gauge(MetricName{"haven_test_active_items"}, 2.0, {});
    recorder->observe_duration(MetricName{"haven_test_duration_seconds"}, 500000us, {});
    haven::presentation::observability::metrics::register_metrics_route(recorder);

    drogon::app().addListener("127.0.0.1", std::uint16_t{18083}).setThreadNum(1);
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

/**
 * @file main.cpp
 * @brief Defines the Haven API process entry point.
 *
 * This file acts as the composition root. It loads validated process
 * configuration, registers presentation routes, and starts the Drogon event
 * loop. Business logic and configuration parsing must remain outside main().
 */

#include "haven/bootstrap/configuration.hpp"
#include "haven/presentation/health/live_controller.hpp"

#include <drogon/HttpAppFramework.h>

#include <cstdlib>
#include <exception>
#include <iostream>

int main() {
    try {
        const haven::bootstrap::ApplicationConfiguration configuration =
            haven::bootstrap::load_configuration_from_environment();

        haven::presentation::health::register_live_route();

        std::cout << "Starting Haven API on " << configuration.http.address << ':'
                  << configuration.http.port << " using "
                  << configuration.http.worker_threads << " HTTP worker thread(s)\n";

        drogon::app()
            .addListener(configuration.http.address, configuration.http.port)
            .setThreadNum(configuration.http.worker_threads)
            .run();

        return EXIT_SUCCESS;
    } catch (const haven::bootstrap::ConfigurationError& error) {
        std::cerr << "Haven configuration error: " << error.what() << '\n';
        return EXIT_FAILURE;
    } catch (const std::exception& error) {
        std::cerr << "Haven failed to start: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
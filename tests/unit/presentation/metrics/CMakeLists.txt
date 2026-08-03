# Haven metrics HTTP endpoint tests.

add_executable(haven_metrics_controller_test
    enabled_main.cpp
    metrics_controller_test.cpp
)
target_link_libraries(haven_metrics_controller_test
    PRIVATE
        haven::presentation
        haven::infrastructure::metrics
        Drogon::Drogon
        Threads::Threads
)
haven_apply_project_options(haven_metrics_controller_test)
add_test(NAME haven_metrics_controller_test COMMAND haven_metrics_controller_test)

add_executable(haven_metrics_disabled_test
    disabled_main.cpp
    metrics_disabled_test.cpp
)
target_link_libraries(haven_metrics_disabled_test
    PRIVATE
        Drogon::Drogon
        Threads::Threads
)
haven_apply_project_options(haven_metrics_disabled_test)
add_test(NAME haven_metrics_disabled_test COMMAND haven_metrics_disabled_test)

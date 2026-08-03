# Haven server runtime support.
add_library(haven_outbox_runtime outbox/outbox_publisher_worker.cpp)
add_library(haven::runtime::outbox ALIAS haven_outbox_runtime)
target_include_directories(haven_outbox_runtime PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_features(haven_outbox_runtime PUBLIC cxx_std_20)
target_link_libraries(haven_outbox_runtime PUBLIC haven::application PRIVATE haven_logging)
haven_apply_project_options(haven_outbox_runtime)

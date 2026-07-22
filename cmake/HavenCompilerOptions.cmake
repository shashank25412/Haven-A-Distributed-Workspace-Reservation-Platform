#------------------------------------------------------------------------------
# Haven Compiler Options
#
# Centralizes compiler features, warning policies, and sanitizer configuration
# for Haven-owned targets.
#
# Third-party dependencies must not inherit these options.
# Each Haven target explicitly opts in by calling:
#
#     haven_apply_project_options(<target>)
#------------------------------------------------------------------------------

function(haven_apply_project_options target_name)

    #--------------------------------------------------------------------------
    # Require C++20 for every Haven target.
    #--------------------------------------------------------------------------
    target_compile_features(
        ${target_name}
        PUBLIC
            cxx_std_20
    )

    #--------------------------------------------------------------------------
    # Compiler warnings
    #
    # Apply only to Haven-owned code.
    #--------------------------------------------------------------------------
    if(MSVC)

        target_compile_options(
            ${target_name}
            PRIVATE

                /W4
                /permissive-
                /EHsc

                $<$<BOOL:${HAVEN_ENABLE_WARNINGS_AS_ERRORS}>:/WX>
        )

    else()

        target_compile_options(
            ${target_name}
            PRIVATE

                -Wall
                -Wextra
                -Wpedantic

                -Wconversion
                -Wsign-conversion

                -Wshadow
                -Wold-style-cast
                -Wnon-virtual-dtor
                -Woverloaded-virtual

                $<$<BOOL:${HAVEN_ENABLE_WARNINGS_AS_ERRORS}>:-Werror>
        )

    endif()

    #--------------------------------------------------------------------------
    # AddressSanitizer + UndefinedBehaviorSanitizer
    #
    # Enabled only through the corresponding CMake option.
    #--------------------------------------------------------------------------
    if(HAVEN_ENABLE_SANITIZERS)

        if(MSVC)

            message(FATAL_ERROR
                "HAVEN_ENABLE_SANITIZERS currently supports GCC and Clang only."
            )

        endif()

        target_compile_options(
            ${target_name}
            PRIVATE

                -fsanitize=address,undefined
                -fno-omit-frame-pointer
        )

        target_link_options(
            ${target_name}
            PRIVATE

                -fsanitize=address,undefined
                -fno-omit-frame-pointer
        )

    endif()

endfunction()
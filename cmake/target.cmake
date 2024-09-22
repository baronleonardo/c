# example for how to use
# ---
# c_create_targets(<target name>
#     PRIVATE_LIBS <libs>
#     PUBLIC_LIBS <libs>
#     TESTING_LIBS <libs>
#     EXTRA_SOURCES <.c/.h files>
# )

function (c_create_targets target_name)
    set(multiValueArgs TARGETS
        PRIVATE_LIBS
        PUBLIC_LIBS
        TESTING_LIBS
        EXTRA_SOURCES
    )
    set(options NO_TESTS_FOR_NOW)
    cmake_parse_arguments(C_TARGET "${options}" ""
                        "${multiValueArgs}" ${ARGN} )

    # create the library
    add_library(${target_name}
        ${target_name}.c
        ${target_name}.h
        ${C_TARGET_EXTRA_SOURCES}
    )

    set_target_properties(${target_name} PROPERTIES PUBLIC_HEADER ${target_name}.h)

    target_link_libraries(${target_name}
        PRIVATE
            ${C_TARGET_PRIVATE_LIBS}
        PUBLIC
            ${C_TARGET_PUBLIC_LIBS}
    )

    # include dirs, libs linking
    target_include_directories(${target_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    # add tests
    if(enable_testing AND (NOT ${C_TARGET_NO_TESTS_FOR_NOW}))
        add_executable(test_${target_name} test_${target_name}.c)
        target_link_libraries(test_${target_name}
            PRIVATE
                ${target_name}
                ${C_TARGET_TESTING_LIBS}
                c::utest
        )

        add_test(
            NAME test_${target_name}
            COMMAND test_${target_name}
        )
    endif()
endfunction()
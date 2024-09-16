project(cbuild)

# add_library(${PROJECT_NAME}
#     cbuild.h
#     cbuild.c
# )
# target_include_directories(${PROJECT_NAME} PUBLIC
#     ${CMAKE_CURRENT_LIST_DIR}
# )

# target_link_libraries(${PROJECT_NAME} PRIVATE utils cprocess c::stb_ds)

# if(enable_testing)
#     add_executable(test_${PROJECT_NAME} test_${PROJECT_NAME}.c)
#     target_link_libraries()
#     add_test(
#         NAME test_${PROJECT_NAME}
#         COMMAND test_${PROJECT_NAME})
# endif()

c_create_targets(${PROJECT_NAME}
    PRIVATE_LIBS utils cprocess c::stb_ds
    PUBLIC_LIBS c::fs utils
    TESTING_LIBS
    EXTRA_SOURCES
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE Shlwapi)
endif()

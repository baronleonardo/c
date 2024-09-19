project(cbuild)

c_create_targets(${PROJECT_NAME}
    PRIVATE_LIBS utils cprocess c::fs
    PUBLIC_LIBS utils c::str c::array
    TESTING_LIBS
    EXTRA_SOURCES
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE Shlwapi)
endif()

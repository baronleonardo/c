project(ccmd)

add_library(${PROJECT_NAME}
    ccmd.h ccmd.c
)
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE c::fs c::dl_loader cbuild c::defer
    PUBLIC utils
)

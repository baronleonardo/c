project(cprocess)

add_library(${PROJECT_NAME}
    cprocess.h
    cprocess.c
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE c::subprocess)

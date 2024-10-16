cmake_minimum_required(VERSION 3.24)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # for clangd
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # enable IPC

include(options.cmake)
include(deps.cmake)
include(cmake/target.cmake)

##### flags
if(WIN32)
    set(cflags
        /W4
        # /WX
        /guard:cf # control flow guard
        /sdl # enable security features
        /analyze # enable static analysis
        /permissive- # standards conformance mode
        /utf-8
        $<$<CONFIG:Debug>:/Zi>
        /wd4200 # C4200 zero-sized array in struct/union
        /wd4820 # C4200 padding in struct/union
    )
    set(lflags
        /guard:cf # control flow guard
    )
else()
    set(cflags
        -Wall -Wextra -Wpedantic
        # -Werror
        # $<$<CONFIG:Debug>:-O1>
        $<$<CONFIG:Debug>:-fsanitize=address>
        $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
        $<$<CONFIG:Release>:-D_FORTIFY_SOURCE=2> # detect runtime buffer overflows
    )
    set(lflags
        $<$<CONFIG:Debug>:-fsanitize=address>
    )
endif()
add_compile_options(${cflags})
add_link_options(${lflags})

if(enable_testing)
    enable_testing()
endif()


##### project
project(c
    LANGUAGES C
    VERSION 0.1
    DESCRIPTION "this is a project/document/unittest management"
)

add_subdirectory(src/ccmd)
add_subdirectory(src/cprocess)
add_subdirectory(src/utils)
add_subdirectory(src/cbuild)

add_executable(${PROJECT_NAME} main.c)
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        ccmd
        $<$<PLATFORM_ID:Linux>:dl>
)
install(TARGETS ${PROJECT_NAME})

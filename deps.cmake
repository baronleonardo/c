include(FetchContent)
include(ExternalProject)

# stb-ds (array and hashmap)
set(stb_ds_path ${CMAKE_BINARY_DIR}/_deps/stb_ds)
if(NOT EXISTS ${stb_ds_path}/stb_ds.h)
    file(DOWNLOAD https://raw.githubusercontent.com/nothings/stb/master/stb_ds.h ${stb_ds_path}/stb_ds.h)
    file(WRITE ${stb_ds_path}/stb_ds.c 
        "#define STBDS_NO_SHORT_NAMES\n"
        "#define STB_DS_IMPLEMENTATION\n"
        "#include \"stb_ds.h\"\n"
    )
endif()
add_library(stb-ds ${stb_ds_path}/stb_ds.h ${stb_ds_path}/stb_ds.c)
target_include_directories(stb-ds PUBLIC ${stb_ds_path})
add_library(c::stb_ds ALIAS stb-ds)


# fs
set(fs_path ${CMAKE_BINARY_DIR}/_deps/fs)
if(NOT EXISTS ${fs_path}/fs.h)
    file(DOWNLOAD https://raw.githubusercontent.com/baronleonardo/cstdlib/main/fs.h ${fs_path}/fs.h)
    file(WRITE ${fs_path}/fs.c 
        "#define CSTDLIB_FS_IMPLEMENTATION\n"
        "#include \"fs.h\"\n"
    )
endif()
add_library(fs ${fs_path}/fs.h ${fs_path}/fs.c)
target_include_directories(fs PUBLIC ${fs_path})
add_library(c::fs ALIAS fs)


# str
set(str_path ${CMAKE_BINARY_DIR}/_deps/str)
if(NOT EXISTS ${str_path}/str.h)
    file(DOWNLOAD https://raw.githubusercontent.com/baronleonardo/cstdlib/main/str.h ${str_path}/str.h)
    file(WRITE ${str_path}/str.c 
        "#define CSTDLIB_STR_IMPLEMENTATION\n"
        "#include \"str.h\"\n"
    )
endif()
add_library(str ${str_path}/str.h ${str_path}/str.c)
target_include_directories(str PUBLIC ${str_path})
add_library(c::str ALIAS str)

# array
set(array_path ${CMAKE_BINARY_DIR}/_deps/array)
if(NOT EXISTS ${array_path}/array.h)
    file(DOWNLOAD https://raw.githubusercontent.com/baronleonardo/cstdlib/main/array.h ${array_path}/array.h)
    file(WRITE ${array_path}/array.c 
        "#define CSTDLIB_ARRAY_IMPLEMENTATION\n"
        "#include \"array.h\"\n"
    )
endif()
add_library(array ${array_path}/array.h ${array_path}/array.c)
target_include_directories(array PUBLIC ${array_path})
add_library(c::array ALIAS array)


# dl_loader
set(dl_loader_path ${CMAKE_BINARY_DIR}/_deps/dl_loader)
if(NOT EXISTS ${dl_loader_path}/dl_loader.h)
    file(DOWNLOAD https://raw.githubusercontent.com/baronleonardo/cstdlib/main/dl_loader.h ${dl_loader_path}/dl_loader.h)
    file(WRITE ${dl_loader_path}/dl_loader.c 
        "#define CSTDLIB_DL_LOADER_IMPLEMENTATION\n"
        "#include \"dl_loader.h\"\n"
    )
endif()
add_library(dl_loader ${dl_loader_path}/dl_loader.h ${dl_loader_path}/dl_loader.c)
target_include_directories(dl_loader PUBLIC ${dl_loader_path})
add_library(c::dl_loader ALIAS dl_loader)

# defer
set(defer_path ${CMAKE_BINARY_DIR}/_deps/defer)
if(NOT EXISTS ${defer_path}/defer.h)
    file(DOWNLOAD https://raw.githubusercontent.com/baronleonardo/cstdlib/main/defer.h ${defer_path}/defer.h)
    file(WRITE ${defer_path}/defer.c 
        "#define CSTDLIB_DERFER_IMPLEMENTATION\n"
        "#include \"defer.h\"\n"
    )
endif()
add_library(defer ${defer_path}/defer.h ${defer_path}/defer.c)
target_include_directories(defer PUBLIC ${defer_path})
add_library(c::defer ALIAS defer)


# utest
set(utest_path ${CMAKE_BINARY_DIR}/_deps/utest)
if(NOT EXISTS ${utest_path}/utest.h)
    file(DOWNLOAD https://raw.githubusercontent.com/sheredom/utest.h/master/utest.h ${utest_path}/utest.h)
    file(WRITE ${utest_path}/utest.c 
        "#include \"utest.h\"\n"
        "UTEST_MAIN()\n"
    )
endif()
add_library(utest ${utest_path}/utest.h ${utest_path}/utest.c)
target_include_directories(utest PUBLIC ${utest_path})
add_library(c::utest ALIAS utest)


# subprocess
set(subprocess_path ${CMAKE_BINARY_DIR}/_deps/subprocess)
if(NOT EXISTS ${subprocess_path}/subprocess.h)
    file(DOWNLOAD https://raw.githubusercontent.com/sheredom/subprocess.h/master/subprocess.h ${subprocess_path}/subprocess.h)
    file(WRITE ${subprocess_path}/subprocess.c 
        "#include \"subprocess.h\"\n"
    )
endif()
add_library(subprocess ${subprocess_path}/subprocess.h ${subprocess_path}/subprocess.c)
target_include_directories(subprocess PUBLIC ${subprocess_path})
add_library(c::subprocess ALIAS subprocess)


# jemalloc
if(NOT WIN32)
    FetchContent_Declare(
        jemalloc
        GIT_REPOSITORY https://github.com/jemalloc/jemalloc.git
        GIT_TAG 5.3.0  # Specify the version you want to use
    )
    # Set where to build the library
    set(JEMALLOC_INSTALL_DIR ${CMAKE_BINARY_DIR}/jemalloc)
    FetchContent_MakeAvailable(jemalloc)
    # Options specific to jemalloc for both platforms
    set(JEMALLOC_BUILD_SHARED OFF CACHE BOOL "Build jemalloc as a shared library")
    set(JEMALLOC_WITH_STATIC_PAGE_SHIFT ON CACHE BOOL "Enable static page shift")
    include(ExternalProject)
    ExternalProject_Add(jemalloc_project
        SOURCE_DIR ${jemalloc_SOURCE_DIR}
        CONFIGURE_COMMAND ${jemalloc_SOURCE_DIR}/autogen.sh --prefix ${JEMALLOC_INSTALL_DIR}
        BUILD_IN_SOURCE 1
        INSTALL_DIR ${JEMALLOC_INSTALL_DIR}
        BUILD_COMMAND make
        INSTALL_COMMAND make install
    )
    # Create a target to link against jemalloc
    add_library(jemalloc INTERFACE)
    target_include_directories(jemalloc INTERFACE ${JEMALLOC_INSTALL_DIR}/include)
    target_link_directories(jemalloc INTERFACE ${JEMALLOC_INSTALL_DIR}/lib)
    target_link_libraries(jemalloc INTERFACE jemalloc_pic)
else()
    add_library(jemalloc INTERFACE)
endif()
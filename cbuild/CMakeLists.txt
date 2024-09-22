project(cbuild)

set(public_libs
    utils
    c::str
    c::array
)

c_create_targets(${PROJECT_NAME}
    PRIVATE_LIBS 
        utils
        cprocess
        c::fs
    PUBLIC_LIBS ${public_libs}
)

# install all public headers needed by this target
foreach(lib ${public_libs})
    get_target_property(source_dir ${lib} INTERFACE_INCLUDE_DIRECTORIES)
    file(GLOB headers "${source_dir}/*.h")
    install(FILES ${headers} TYPE INCLUDE)
endforeach()
get_target_property(headers ${PROJECT_NAME} PUBLIC_HEADER)
install(FILES ${headers} TYPE INCLUDE)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE Shlwapi)
endif()

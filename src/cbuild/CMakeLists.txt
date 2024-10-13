project(cbuild)

set(public_libs
    c::str
    c::array
    utils
    )
    
set(private_libs
    utils
    cprocess
    c::fs
    c::dl_loader
    jemalloc
)

c_create_targets(${PROJECT_NAME}
    TYPE            SHARED
    PRIVATE_LIBS    ${private_libs}
    PUBLIC_LIBS     ${public_libs}
)
install(TARGETS ${PROJECT_NAME})
target_compile_definitions(${PROJECT_NAME} PRIVATE __C_BUILD_DLL__)

# install all public headers needed by this target
foreach(lib ${public_libs})
    get_target_property(source_dir ${lib} INTERFACE_INCLUDE_DIRECTORIES)
    file(GLOB headers "${source_dir}/*.h")
    install(FILES ${headers} TYPE INCLUDE)
endforeach()

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE Shlwapi)
endif()

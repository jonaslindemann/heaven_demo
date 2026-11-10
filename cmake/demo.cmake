# add_demo(<name> SOURCES <files...> [ASSETS <dir>])
#
# Builds a demo executable linked with the demo engine. ivf2's shaders and
# fonts and the demo's asset directory are copied next to the executable
# (as assets/), only when they have changed.
function(add_demo name)
    cmake_parse_arguments(ARG "" "ASSETS" "SOURCES" ${ARGN})

    add_executable(${name} ${ARG_SOURCES})
    target_link_libraries(${name} PRIVATE demo_engine)
    target_precompile_headers(${name} PRIVATE ${DEMO_ENGINE_PCH})
    set_target_properties(${name} PROPERTIES
        FOLDER "demos"
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${name}>"
    )

    foreach(_dir IN LISTS IVF2_RUNTIME_DIRS)
        get_filename_component(_dirname "${_dir}" NAME)
        add_custom_command(TARGET ${name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${_dir}" "$<TARGET_FILE_DIR:${name}>/${_dirname}"
        )
    endforeach()

    if(ARG_ASSETS)
        # Lets development builds read assets from the project, see DemoConfig::resolve()
        target_compile_definitions(${name} PRIVATE DEMO_ASSET_SOURCE_DIR="${ARG_ASSETS}")
        add_custom_command(TARGET ${name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${ARG_ASSETS}" "$<TARGET_FILE_DIR:${name}>/assets"
            COMMENT "Copying assets of ${name}"
        )
    endif()
endfunction()

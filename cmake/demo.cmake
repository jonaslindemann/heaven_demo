# add_demo(<name> SOURCES <files...> [ASSETS <dir>])
#
# Builds a demo executable on ivfmusic (ivf2::ivfmusic) with ivfmusic_add_executable():
# the demo's asset directory is copied next to the executable as assets/ and read
# from the project during development (IVFMUSIC_ASSET_SOURCE_DIR). ivf2's shaders
# and fonts are copied as well. All copies are made only when files have changed.
function(add_demo name)
    cmake_parse_arguments(ARG "" "ASSETS" "SOURCES" ${ARGN})

    if(ARG_ASSETS)
        ivfmusic_add_executable(${name} SOURCES ${ARG_SOURCES} ASSETS ${ARG_ASSETS})
    else()
        ivfmusic_add_executable(${name} SOURCES ${ARG_SOURCES})
    endif()
    target_precompile_headers(${name} PRIVATE ${DEMO_PCH})
    set_target_properties(${name} PROPERTIES FOLDER "demos")

    foreach(_dir IN LISTS IVF2_RUNTIME_DIRS)
        get_filename_component(_dirname "${_dir}" NAME)
        add_custom_command(TARGET ${name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${_dir}" "$<TARGET_FILE_DIR:${name}>/${_dirname}"
        )
    endforeach()
endfunction()

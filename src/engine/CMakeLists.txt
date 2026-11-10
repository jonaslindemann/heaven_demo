add_library(demo_engine STATIC
    blend_material.cpp
    blend_material.h
    demo_config.cpp
    demo_config.h
    demo_window.cpp
    demo_window.h
    glow_effect.cpp
    glow_effect.h
    guided_camera.cpp
    guided_camera.h
    music_clock.cpp
    music_clock.h
    param_timeline.cpp
    param_timeline.h
    renoise_loader.cpp
    renoise_loader.h
    song_data.cpp
    song_data.h
)

find_package(miniz CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)

target_include_directories(demo_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(demo_engine PUBLIC ivf2::ivf2 PRIVATE miniz::miniz pugixml::pugixml)
target_precompile_headers(demo_engine PRIVATE pch.h)
if(MSVC)
    target_compile_options(demo_engine PUBLIC /utf-8)
endif()
set_target_properties(demo_engine PROPERTIES FOLDER "engine")

set(DEMO_ENGINE_PCH "${CMAKE_CURRENT_SOURCE_DIR}/pch.h" CACHE INTERNAL "")

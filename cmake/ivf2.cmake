# Imports the prebuilt ivf2 libraries from an external ivf2 checkout (IVF2_DIR)
# as the ivf2::ivf2 interface target. Build ivf2 itself first, in its own tree:
#   cmake --build build-debug --config Debug / build-release --config Release
#
# Expects headers in ${IVF2_DIR}/include and libraries in ${IVF2_DIR}/lib/<Config>
# (multi-config generators) or ${IVF2_DIR}/lib (single-config), Debug libs with a 'd' postfix.

set(IVF2_DIR "${PROJECT_SOURCE_DIR}/../ivf2" CACHE PATH "Path to the ivf2 source tree")
get_filename_component(IVF2_DIR "${IVF2_DIR}" ABSOLUTE)
if(NOT EXISTS "${IVF2_DIR}/include/ivf/gl.h")
    message(FATAL_ERROR "ivf2 not found at '${IVF2_DIR}'. Set -DIVF2_DIR=<path to ivf2>.")
endif()

find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(glew REQUIRED)
find_package(OpenGL REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(Freetype REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(RtMidi CONFIG REQUIRED)

# Link order matters for static libs: dependents before dependencies
set(_ivf2_libs ivfui ivf ivfmath generator glad)
set(_ivf2_targets)

foreach(_lib IN LISTS _ivf2_libs)
    find_library(IVF2_${_lib}_LIBRARY_RELEASE NAMES ${_lib}
        PATHS "${IVF2_DIR}/lib/Release" "${IVF2_DIR}/lib" NO_DEFAULT_PATH)
    find_library(IVF2_${_lib}_LIBRARY_DEBUG NAMES ${_lib}d
        PATHS "${IVF2_DIR}/lib/Debug" "${IVF2_DIR}/lib" NO_DEFAULT_PATH)

    if(NOT IVF2_${_lib}_LIBRARY_RELEASE AND NOT IVF2_${_lib}_LIBRARY_DEBUG)
        message(FATAL_ERROR "ivf2 library '${_lib}' not found under ${IVF2_DIR}/lib. Build ivf2 first.")
    endif()

    add_library(ivf2::${_lib} STATIC IMPORTED)
    if(IVF2_${_lib}_LIBRARY_RELEASE)
        set_property(TARGET ivf2::${_lib} APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(ivf2::${_lib} PROPERTIES IMPORTED_LOCATION_RELEASE "${IVF2_${_lib}_LIBRARY_RELEASE}")
    else()
        message(WARNING "ivf2 Release library '${_lib}' not found; Release builds will fail to link.")
    endif()
    if(IVF2_${_lib}_LIBRARY_DEBUG)
        set_property(TARGET ivf2::${_lib} APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(ivf2::${_lib} PROPERTIES IMPORTED_LOCATION_DEBUG "${IVF2_${_lib}_LIBRARY_DEBUG}")
    else()
        message(WARNING "ivf2 Debug library '${_lib}' not found; Debug builds will fail to link.")
    endif()
    set_target_properties(ivf2::${_lib} PROPERTIES
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
        MAP_IMPORTED_CONFIG_MINSIZEREL Release
    )
    list(APPEND _ivf2_targets ivf2::${_lib})
endforeach()

add_library(ivf2 INTERFACE)
add_library(ivf2::ivf2 ALIAS ivf2)

target_link_libraries(ivf2 INTERFACE
    ${_ivf2_targets}
    imgui::imgui
    PNG::PNG
    JPEG::JPEG
    ZLIB::ZLIB
    glfw
    GLEW::GLEW
    OpenGL::GL
    Freetype::Freetype
    RtMidi::rtmidi
    assimp::assimp
    nlohmann_json::nlohmann_json
)

target_include_directories(ivf2 INTERFACE
    ${IVF2_DIR}/include
    ${Stb_INCLUDE_DIR}
)

if(NOT MSVC)
    target_compile_definitions(ivf2 INTERFACE $<$<CONFIG:Debug>:_DEBUG>)
endif()

# Runtime data shipped with ivf2 (copied next to the executable)
set(IVF2_RUNTIME_DIRS "${IVF2_DIR}/bin/shaders;${IVF2_DIR}/bin/fonts")

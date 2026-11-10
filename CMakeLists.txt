cmake_minimum_required(VERSION 3.26)

# Locate the vcpkg toolchain. A standalone vcpkg is preferred over the copy
# bundled with Visual Studio (which VCPKG_ROOT often points at).
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE OR CMAKE_TOOLCHAIN_FILE MATCHES "Microsoft Visual Studio")
    set(_vcpkg_candidates)
    if(DEFINED ENV{VCPKG_ROOT} AND NOT "$ENV{VCPKG_ROOT}" MATCHES "Microsoft Visual Studio")
        list(APPEND _vcpkg_candidates "$ENV{VCPKG_ROOT}")
    endif()
    if(WIN32)
        list(APPEND _vcpkg_candidates "c:/vcpkg" "e:/vcpkg")
    else()
        list(APPEND _vcpkg_candidates "$ENV{HOME}/vcpkg" "/usr/local/vcpkg")
    endif()
    if(DEFINED ENV{VCPKG_ROOT})
        list(APPEND _vcpkg_candidates "$ENV{VCPKG_ROOT}")
    endif()

    foreach(_dir IN LISTS _vcpkg_candidates)
        if(EXISTS "${_dir}/scripts/buildsystems/vcpkg.cmake")
            set(CMAKE_TOOLCHAIN_FILE "${_dir}/scripts/buildsystems/vcpkg.cmake"
                CACHE STRING "Vcpkg toolchain file" FORCE)
            message(STATUS "Using vcpkg at: ${_dir}")
            break()
        endif()
    endforeach()

    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        message(WARNING "vcpkg toolchain file not found. Set VCPKG_ROOT or CMAKE_TOOLCHAIN_FILE.")
    endif()
endif()

# Shared vcpkg_installed/ in the project root, used by both build-debug and build-release
set(VCPKG_INSTALLED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed" CACHE PATH "vcpkg installed directory")

project(heaven_demo LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

# Executables: bin/debug and bin/release
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}/bin/debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/bin/release")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PROJECT_SOURCE_DIR}/bin/relwithdebinfo")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${PROJECT_SOURCE_DIR}/bin/minsizerel")

# Prebuilt ivf2 libraries from the external ivf2 checkout (../ivf2 by default)
include(cmake/ivf2.cmake)

# Reusable demo engine, and one directory per demo built with add_demo()
include(cmake/demo.cmake)
add_subdirectory(src/engine)

add_subdirectory(demos/heaven)
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT heaven_demo)

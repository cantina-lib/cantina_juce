# - Find the LADSPA v2 (LV2) development library
#
# Based on FindFFTW.cmake:
#   Copyright (c) 2015, Wenzel Jakob
#   https://github.com/wjakob/layerlab/blob/master/cmake/FindFFTW.cmake, commit 4d58bfdc28891b4f9373dfe46239dda5a0b561c6
#   Copyright (c) 2017, Patrick Bos
#   https://github.com/egpbos/findFFTW/blob/master/FindFFTW.cmake
#
# Usage:
#   find_package(LV2 [REQUIRED] [QUIET])
#
# It sets the following variables:
#   LV2_FOUND                  ... true if lv2 is found on the system
#   LV2_SCHEMAS_DIR           ... lv2 schemas directory paths (ttf validation templates)
# It does NOT set the following variables:
#   LV2_LIBRARIES              ... because LV2 is header-only
#
# The following variables will be checked by the function
#   LV2_USE_STATIC_LIBS        ... if true, only static libraries are found, otherwise both static and shared.
#   LV2_ROOT                   ... if set, the libraries are exclusively searched
#                                   under this path
#
include(Utils)

if (NOT LV2_ROOT AND DEFINED ENV{LV2DIR})
    set(LV2_ROOT $ENV{LV2DIR})
endif ()

# Check if we can use PkgConfig
find_package(PkgConfig)

#Determine from PKG
if (PKG_CONFIG_FOUND AND NOT LV2_ROOT)
    pkg_check_modules(PKG_LV2 QUIET "LV2")
endif ()

#Check whether to search static or dynamic libs
set(CMAKE_FIND_LIBRARY_SUFFIXES_SAV ${CMAKE_FIND_LIBRARY_SUFFIXES})

if (${LV2_USE_STATIC_LIBS})
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
else ()
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_SAV})
endif ()

if (LV2_ROOT)
    #find includes
    find_path(LV2_INCLUDE_DIRS
            NAMES "lv2"
            PATHS ${LV2_ROOT}
            PATH_SUFFIXES "include"
            NO_DEFAULT_PATH
            )

    #find schemas
    find_path(LV2_SCHEMAS_DIRS_CHILD
            NAMES "lv2" "schemas" "schemas.lv2"
            PATHS ${LV2_ROOT}
            PATH_SUFFIXES "lib"
            NO_DEFAULT_PATH
            )


else ()
    find_path(LV2_INCLUDE_DIRS
            NAMES "lv2"
            PATHS ${PKG_LV2_INCLUDE_DIRS} ${INCLUDE_INSTALL_DIR}
            PATH_SUFFIXES "include"
            )

    # todo: better arguments.
    find_path(LV2_SCHEMAS_DIRS_CHILD
            NAMES "manifest.ttl" "dcs.ttl"
            PATHS ${PKG_LV2_LIBRARY_DIRS} ${LIB_INSTALL_DIR} "/usr/lib"
            PATH_SUFFIXES "lib" "lv2" "lv2/schemas.lv2"
            )

endif (LV2_ROOT)

get_filename_component(LV2_SCHEMAS_DIRS_PARENT ${LV2_SCHEMAS_DIRS_CHILD} DIRECTORY)
# get_subdirectories(LV2_SCHEMAS_DIR ${LV2_SCHEMAS_DIRS_PARENT})
set(LV2_SCHEMAS_DIR ${LV2_SCHEMAS_DIRS_PARENT} CACHE INTERNAL "LV2 schemas parent directory")


add_library(LV2 INTERFACE IMPORTED)
set_target_properties(LV2
        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${LV2_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${LV2_LIBRARIES}"
        )

set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_SAV})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LV2
        REQUIRED_VARS LV2_INCLUDE_DIRS
        HANDLE_COMPONENTS
        )

mark_as_advanced(
        LV2_LIBRARIES
        LV2_INCLUDE_DIRS
       LV2_SCHEMAS_DIR 
)

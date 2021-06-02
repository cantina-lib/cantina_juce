cmake_minimum_required(VERSION 3.15)

project(cantina_plugin)


# see: https://cmake.org/cmake/help/v2.9/manual/cmake-variables.7.html
message("CMAKE_VERSION          : " ${CMAKE_VERSION})
message("CMAKE_SYSTEM_NAME      : " ${CMAKE_SYSTEM_NAME})
message("CMAKE_SYSTEM           : " ${CMAKE_SYSTEM})
message("CMAKE_SYSTEM_PROCESSOR : " ${CMAKE_SYSTEM_PROCESSOR})

# spell out the directories
message("CMAKE_BINARY_DIR       : " ${CMAKE_BINARY_DIR})
message("CMAKE_SOURCE_DIR       : " ${CMAKE_SOURCE_DIR})
message("CMAKE_HOME_DIRECTORY   : " ${CMAKE_HOME_DIRECTORY})
message("PROJECT_BINARY_DIR     : " ${PROJECT_BINARY_DIR})
message("PROJECT_SOURCE_DIR     : " ${PROJECT_SOURCE_DIR})

set(CANTINA_PLUGIN_LV2_DIR ${PROJECT_SOURCE_DIR}/cantina_lv2)
set(CANTINA_PLUGIN_JUCE_DIR ${PROJECT_SOURCE_DIR}/cantina_juce)
set(CANTINA_PLUGIN_OUTPUT_DIR ${PROJECT_BINARY_DIR}/cantina_plugin)

set(CANTINA_URI http://lv2plug.in/selfs/cantina)

# Add dependencies first so that they are valid in the plug-ins.
# cantina
set(CANTINA_DIR ${PROJECT_SOURCE_DIR}/cantina)
add_subdirectory(${CANTINA_DIR})

#
add_subdirectory(${CANTINA_PLUGIN_LV2_DIR})
add_subdirectory(${CANTINA_PLUGIN_JUCE_DIR})


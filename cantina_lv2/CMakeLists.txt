cmake_minimum_required(VERSION 3.15)

project(cantina_lv2)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/modules/cmake)

set(CANTINA_LV2_SOURCE_DIR ${PROJECT_SOURCE_DIR}/source)
set(CANTINA_LV2_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(CANTINA_LV2_DATA_DIR ${PROJECT_SOURCE_DIR}/data)

# LV2
find_package(LV2 REQUIRED)

set(CANTINA_LV2_FILES
        ${CANTINA_LV2_SOURCE_DIR}/cantina_plugin.cpp
        ${CANTINA_LV2_INCLUDE_DIR}/cantina_plugin.hpp
        )

set(LV2_DEFINITIONS
        PUBLIC
        CANTINA_URI="${CANTINA_URI}"
        )

add_library(${PROJECT_NAME} SHARED ${CANTINA_LV2_FILES})

# no special flags for this one, since it's mess.
target_compile_options(${PROJECT_NAME} PRIVATE "")# ${CANTINA_CXX_FLAGS})
target_compile_features(${PROJECT_NAME} PRIVATE ${CANTINA_CXX_STANDARD})

target_include_directories(${PROJECT_NAME} PUBLIC ${CANTINA_LV2_INCLUDE_DIR})

target_compile_definitions(${PROJECT_NAME} ${LV2_DEFINITIONS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${CANTINA_LIBRARIES} ${LV2_LIBRARIES})

# BUILD SPECIFICS
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CANTINA_PLUGIN_OUTPUT_DIR})
configure_file(${CANTINA_LV2_DATA_DIR}/manifest.ttl ${CANTINA_PLUGIN_OUTPUT_DIR}/manifest.ttl)
configure_file(${CANTINA_LV2_DATA_DIR}/cantina.ttl ${CANTINA_PLUGIN_OUTPUT_DIR}/cantina.ttl)

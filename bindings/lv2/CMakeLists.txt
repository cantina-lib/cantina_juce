cmake_minimum_required(VERSION 3.15)

project(cantina_lv2)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/modules/cmake)
set(CANTINA_LV2_SOURCE_DIR ${PROJECT_SOURCE_DIR}/source)
set(CANTINA_LV2_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(CANTINA_LV2_DATA_DIR ${PROJECT_SOURCE_DIR}/data)

set(CANTINA_LV2_MANIFEST ${CANTINA_LV2_DATA_DIR}/manifest.ttl)
set(CANTINA_LV2_INFO ${CANTINA_LV2_DATA_DIR}/cantina.ttl)

set(CANTINA_LV2_URI https://github.com/cantina-lib)

set(CANTINA_LV2_OUTPUT_PATH ${PROJECT_BINARY_DIR}/cantina.lv2)

set(CANTINA_LV2_FILES
        ${CANTINA_LV2_SOURCE_DIR}/cantina_plugin.cpp
        ${CANTINA_LV2_INCLUDE_DIR}/cantina_plugin.hpp
        )

include(LV2Utils)

add_lv2_plugin(${PROJECT_NAME}
        ${CANTINA_LV2_URI}
        ${CANTINA_LV2_MANIFEST}
        ${CANTINA_LV2_INFO}
        ${CANTINA_LV2_OUTPUT_PATH}
        )

target_sources(${PROJECT_NAME} PRIVATE ${CANTINA_LV2_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CANTINA_LV2_INCLUDE_DIR})

# no special flags for this one, since it's mess.
target_compile_options(${PROJECT_NAME} PRIVATE "")# ${CANTINA_CXX_FLAGS})
target_compile_features(${PROJECT_NAME} PRIVATE ${CANTINA_CXX_STANDARD})

target_link_libraries(${PROJECT_NAME} PUBLIC ${CANTINA_LIBRARIES})
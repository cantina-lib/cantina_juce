cmake_minimum_required(VERSION 3.15)

project(cantina_juce)

set(CANTINA_JUCE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/source)
set(CANTINA_JUCE_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(CANTINA_JUCE_DATA_DIR ${PROJECT_SOURCE_DIR}/data)
set(THIRDPARTY_DIR ${PROJECT_SOURCE_DIR}/third-party)

# JUCE
set(JUCE_DIR ${THIRDPARTY_DIR}/JUCE)
add_subdirectory(${JUCE_DIR})

# see: https://github.com/juce-framework/JUCE/blob/master/examples/CMake/AudioPlugin/CMakeLists.txt
set(JUCE_LIBRARIES
        PRIVATE
            juce::juce_audio_utils
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags
        )

set(JUCE_DEFINITIONS
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            CANTINA_URI="${CANTINA_URI}"
        )

set(CANTINA_JUCE_FILES
        # todo
        )

# taken from: https://jatinchowdhury18.medium.com/building-lv2-plugins-with-juce-and-cmake-d1f8937dbac3
set(JUCE_FORMATS AU VST3 Standalone LV2)

juce_add_plugin(${PROJECT_NAME}
        COMPANY_NAME cantina-lib
        FORMATS ${JUCE_FORMATS}
        VERSION ${CANTINA_VERSION}
        ProductName "Cantina"
        MICROPHONE_PERMISSION_ENABLED TRUE
        LV2_URI ${CANTINA_URI}
        LV2_SHARED_LIBRARY_NAME ${PROJECT_NAME}
)

target_sources(${PROJECT_NAME} PRIVATE ${CANTINA_JUCE_FILES})

# no special flags for this one, since it's mess.
target_compile_options(${PROJECT_NAME} PRIVATE ${CANTINA_CXX_FLAGS})
target_compile_features(${PROJECT_NAME} PRIVATE ${CANTINA_CXX_STANDARD})
target_include_directories(${PROJECT_NAME} PUBLIC ${CANTINA_JUCE_INCLUDE_DIR})

target_compile_definitions(${PROJECT_NAME} ${JUCE_DEFINITIONS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${CANTINA_LIBRARIES} ${JUCE_LIBRARIES})
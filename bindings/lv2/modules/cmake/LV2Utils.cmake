include_guard()
# based on pd.build and JUCEUtils.cmake
# todo: add refs

set(VALIDATE_LV2 true)

set(LV2_PLUGIN_LIB_DIR)
set(LV2_PLUGIN_RES_DIR)

set(LV2_PLUGIN_LIB_DIR content/binary)
set(LV2_PLUGIN_RES_DIR content/resources)

function(validate_ttl)
        cmake_parse_arguments(
                ARGS
                "" # empty options
                "SCHEMAS_DIR" # dir of LV2 install
                "TTL_SRC" # list of source ttl
                ${ARGN}
        )
        file(GLOB_RECURSE SCHEMAS ${ARGS_SCHEMAS_DIR}/*.ttl)
        set(TTL_FILES ${SCHEMAS})
        list(APPEND TTL_FILES ${ARGS_TTL_SRC}) 
        # set(LV2_VALIDATE_COMMAND sord_validate "$(find ${LV2_SCHEMAS_DIRS} -name '*.ttl') $(find ${PLUGIN_DATA_PATH} -name '*.ttl')")
        # message(STATUS ${LV2_VALIDATE_COMMAND})
        execute_process(COMMAND sord_validate ${TTL_FILES}
                OUTPUT_VARIABLE LV2_SORD_OUTPUT
                ERROR_VARIABLE LV2_SORD_ERROR
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ECHO_ERROR_VARIABLE
                COMMAND_ERROR_IS_FATAL ANY
        )
endfunction(validate_ttl)

function(add_lv2_plugin
        PLUGIN_NAME
        PLUGIN_URI
        PLUGIN_MANIFEST
        PLUGIN_INFO
        PLUGIN_OUTPUT_PATH
        )

    add_library(${PLUGIN_NAME} SHARED)

    set(LV2_DEFINITIONS
            CANTINA_URI="${PLUGIN_URI}"
            )
    # LV2
    find_package(LV2 REQUIRED)

    target_compile_definitions(${PLUGIN_NAME} PUBLIC ${LV2_DEFINITIONS})
    target_link_libraries(${PLUGIN_NAME} PUBLIC ${LV2_LIBRARIES})

    # BUILD SPECIFICS
    set_target_properties(${PLUGIN_NAME} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_PATH}/${LV2_PLUGIN_LIB_DIR}"
            PREFIX "" # to match regardless of platform.
            )

    configure_file(${PLUGIN_MANIFEST} ${PLUGIN_OUTPUT_PATH}/manifest.ttl)
    configure_file(${PLUGIN_INFO} ${PLUGIN_OUTPUT_PATH}/${PLUGIN_NAME}.ttl)

    set(TTL_SRC ${PLUGIN_MANIFEST} ${PLUGIN_INFO})
    if (${VALIDATE_LV2})
        validate_ttl(TTL_SRC ${TTL_SRC} SCHEMAS_DIR ${LV2_SCHEMAS_DIR})
    endif()
endfunction(add_lv2_plugin)
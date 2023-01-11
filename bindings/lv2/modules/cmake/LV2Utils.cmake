include_guard()
# based on pd.build and JUCEUtils.cmake
# todo: add refs

set(VALIDATE_LV2 true)

set(LV2_PLUGIN_LIB_DIR)
set(LV2_PLUGIN_RES_DIR)

set(LV2_PLUGIN_LIB_DIR Contents/Binary)
set(LV2_PLUGIN_RES_DIR Contents/Resources)
set(LV2_PLUGIN_INC_DIR Contents/Headers)

function(validate_ttl)
        cmake_parse_arguments(
                ARGS
                "" # empty options
                "SCHEMAS_DIR" # dir of LV2 install
                "TTL" # list of ttl files
                ${ARGN}
        )
        file(GLOB_RECURSE SCHEMAS ${ARGS_SCHEMAS_DIR}/*.ttl)
        set(TTL_FILES ${SCHEMAS})
        list(APPEND TTL_FILES ${ARGS_TTL})
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

function(add_lv2_plugin)

    cmake_parse_arguments(
            ARGS
            "" # empty options
            "NAME;URI;OUTPUT_PATH" #
            "TTL_SRC;INC" # list of source ttl and includes
            ${ARGN}
    )

    add_library(${ARGS_NAME} SHARED)

    set(LV2_DEFINITIONS
        PLUGIN_URI="${ARGS_URI}"
            )
    # LV2
    find_package(LV2 REQUIRED)

    target_compile_definitions(${ARGS_NAME} PUBLIC ${LV2_DEFINITIONS})
    target_link_libraries(${ARGS_NAME} PUBLIC LV2)

    # BUILD SPECIFICS
    set_target_properties(${ARGS_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${ARGS_OUTPUT_PATH}/${LV2_PLUGIN_LIB_DIR}"
            PREFIX "" # to match regardless of platform.
            )

    foreach(TTL_TEMPLATE ${ARGS_TTL_SRC})
        cmake_path(GET TTL_TEMPLATE FILENAME TTL_TEMPLATE_FILENAME)
        string(REGEX REPLACE "\.in$" "" TTL_FILENAME ${TTL_TEMPLATE_FILENAME})
        set(TTL_FILE ${ARGS_OUTPUT_PATH}/${TTL_FILENAME})
        list(APPEND TTL_FILES ${TTL_FILE})
        set(LIB_EXT ${CMAKE_SHARED_LIBRARY_SUFFIX})
        configure_file(${TTL_TEMPLATE} ${TTL_FILE})
    endforeach()
    if (${VALIDATE_LV2})
        validate_ttl(TTL ${TTL_FILES} SCHEMAS_DIR ${LV2_SCHEMAS_DIR})
    endif(${VALIDATE_LV2})

endfunction(add_lv2_plugin)


function(install_lv2_plugin
        )
    cmake_parse_arguments(
            ARGS
            "" # empty options
            "NAME" # dir of LV2 install
            "TTL_SRC;INC" # list of source ttl and includes
            ${ARGN}
    )
    # Must have been 'add'ed first
    if(NOT ARGS_NAME)
        message(FATAL_ERROR "No plug-in name given.")
    endif(NOT ARGS_NAME)
    install(
        TARGETS ${ARGS_NAME}
        DESTINATION "lib/lv2/${ARGS_NAME}/${LV2_PLUGIN_LIB_DIR}"
            )
    install(
        FILES ${ARGS_TTL_SRC}
        DESTINATION "lib/lv2/${ARGS_NAME}"
    )
    install(
        FILES ${ARGS_INC}
        DESTINATION "lib/lv2/${ARGS_NAME}/${LV2_PLUGIN_INC_DIR}"
    )
endfunction(install_lv2_plugin)

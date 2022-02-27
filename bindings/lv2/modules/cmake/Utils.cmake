include_guard()

function(get_subdirectories RESULT DIRECTORY)
    #from: https://stackoverflow.com/a/7788165
    file(GLOB CHILDREN RELATIVE ${DIRECTORY} ${DIRECTORY}/*)
    set(DIRLIST "")
    foreach(CHILD ${CHILDREN})
        if(IS_DIRECTORY ${DIRECTORY}/${CHILD})
        list(APPEND DIRLIST ${DIRECTORY}/${CHILD})
        endif()
    endforeach()
    set(${RESULT} ${DIRLIST} PARENT_SCOPE)
endfunction(get_subdirectories)
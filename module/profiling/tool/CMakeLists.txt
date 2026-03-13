file(GLOB ENTRIES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)

foreach(ENTRY ${ENTRIES})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${ENTRY} # 目录
       AND NOT ENTRY MATCHES "^\\."                      # 非当前目录
       AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${ENTRY}/CMakeLists.txt) # 包含CMakeLists.txt
        
        message(STATUS "Auto adding subdirectory: ${ENTRY}")
        add_subdirectory(${ENTRY})
    endif()
endforeach()

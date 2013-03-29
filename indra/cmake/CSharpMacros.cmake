# - This is a support module for easy Mono/C# handling with CMake
# It defines the following macros:
#
# ADD_CS_LIBRARY (<target> <source>)
# ADD_CS_EXECUTABLE (<target> <source>)
# INSTALL_GAC (<target>)
#
# Note that the order of the arguments is important.
#
# You can optionally set the variable CS_FLAGS to tell the macros whether
# to pass additional flags to the compiler. This is particularly useful to
# set assembly references, unsafe code, etc... These flags are always reset
# after the target was added so you don't have to care about that.
#
# copyright (c) 2007 Arno Rehn arno@arnorehn.de
#
# Redistribution and use is allowed according to the terms of the GPL license.


# ----- support macros -----
MACRO(GET_CS_LIBRARY_TARGET_DIR)
        IF (NOT LIBRARY_OUTPUT_PATH)
                SET(CS_LIBRARY_TARGET_DIR ${CMAKE_CURRENT_BINARY_DIR})
        ELSE (NOT LIBRARY_OUTPUT_PATH)
                SET(CS_LIBRARY_TARGET_DIR ${LIBRARY_OUTPUT_PATH})
        ENDIF (NOT LIBRARY_OUTPUT_PATH)
ENDMACRO(GET_CS_LIBRARY_TARGET_DIR)

MACRO(GET_CS_EXECUTABLE_TARGET_DIR)
        IF (NOT EXECUTABLE_OUTPUT_PATH)
                SET(CS_EXECUTABLE_TARGET_DIR ${CMAKE_CURRENT_BINARY_DIR})
        ELSE (NOT EXECUTABLE_OUTPUT_PATH)
                SET(CS_EXECUTABLE_TARGET_DIR ${EXECUTABLE_OUTPUT_PATH})
        ENDIF (NOT EXECUTABLE_OUTPUT_PATH)
ENDMACRO(GET_CS_EXECUTABLE_TARGET_DIR)

MACRO(MAKE_PROPER_FILE_LIST)
        FOREACH(file ${ARGN})
                # first assume it's a relative path
                FILE(GLOB globbed ${CMAKE_CURRENT_SOURCE_DIR}/${file})
                IF(globbed)
                        FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${file} native)
                ELSE(globbed)
                        FILE(TO_NATIVE_PATH ${file} native)
                ENDIF(globbed)
                SET(proper_file_list ${proper_file_list} ${native})
                SET(native "")
        ENDFOREACH(file)
ENDMACRO(MAKE_PROPER_FILE_LIST)
# ----- end support macros -----

MACRO(ADD_CS_LIBRARY target)
        GET_CS_LIBRARY_TARGET_DIR()
        
        SET(target_DLL "${CS_LIBRARY_TARGET_DIR}/${target}.dll")
        MAKE_PROPER_FILE_LIST(${ARGN})
        FILE(RELATIVE_PATH relative_path ${CMAKE_BINARY_DIR} ${target_DLL})
        
        SET(target_KEY "${CMAKE_CURRENT_SOURCE_DIR}/${target}.key")
        SET(target_CS_FLAGS "${CS_FLAGS}")
        IF(${target}_CS_FLAGS)
                LIST(APPEND target_CS_FLAGS ${${target}_CS_FLAGS})
        ENDIF(${target}_CS_FLAGS)
        IF(EXISTS ${target_KEY})
                LIST(APPEND target_CS_FLAGS -keyfile:${target_KEY})
        ENDIF(EXISTS ${target_KEY})

        FOREACH(ref ${${target}_REFS})
                SET(ref_DLL ${CMAKE_CURRENT_BINARY_DIR}/${ref}.dll)
                IF(EXISTS ${ref_DLL})
                        LIST(APPEND target_CS_FLAGS -r:${ref_DLL})
                ELSE(EXISTS ${ref_DLL})
                        LIST(APPEND target_CS_FLAGS -r:${ref})
                ENDIF(EXISTS ${ref_DLL})
        ENDFOREACH(ref ${${target}_REFS})

        ADD_CUSTOM_COMMAND (OUTPUT ${target_DLL}
                COMMAND ${MCS_EXECUTABLE} ${target_CS_FLAGS} -out:${target_DLL} -target:library ${proper_file_list}
                MAIN_DEPENDENCY ${proper_file_list}
                DEPENDS ${ARGN}
                COMMENT "Building ${relative_path}")
        ADD_CUSTOM_TARGET (${target} ALL DEPENDS ${target_DLL})

        FOREACH(ref ${${target}_REFS})
                GET_TARGET_PROPERTY(is_target ${ref} TYPE)
                IF(is_target)
                        ADD_DEPENDENCIES(${target} ${ref})
                ENDIF(is_target)
        ENDFOREACH(ref ${${target}_REFS})

        SET(relative_path "")
        SET(proper_file_list "")
ENDMACRO(ADD_CS_LIBRARY)

MACRO(ADD_CS_EXECUTABLE target)
        GET_CS_EXECUTABLE_TARGET_DIR()
        
        # Seems like cmake doesn't like the ".exe" ending for custom commands.
        # If we call it ${target}.exe, 'make' will later complain about a missing rule.
        # Create a fake target instead.
        SET(target_EXE "${CS_EXECUTABLE_TARGET_DIR}/${target}.exe")
        SET(target_TOUCH "${CS_EXECUTABLE_TARGET_DIR}/${target}.exe-built")
        GET_DIRECTORY_PROPERTY(clean ADDITIONAL_MAKE_CLEAN_FILES)
        LIST(APPEND clean ${target}.exe)
        SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${clean}")
        MAKE_PROPER_FILE_LIST(${ARGN})
        FILE(RELATIVE_PATH relative_path ${CMAKE_BINARY_DIR} ${target_EXE})
        SET(target_CS_FLAGS "${CS_FLAGS}")
        
        FOREACH(ref ${${target}_REFS})
                SET(ref_DLL ${CMAKE_CURRENT_SOURCE_DIR}/${ref}.dll)
                IF(EXISTS ${ref_DLL})
                        LIST(APPEND target_CS_FLAGS -r:${ref_DLL})
                ELSE(EXISTS ${ref_DLL})
                        LIST(APPEND target_CS_FLAGS -r:${ref})
                ENDIF(EXISTS ${ref_DLL})
        ENDFOREACH(ref ${${target}_REFS})

        ADD_CUSTOM_COMMAND (OUTPUT "${target_TOUCH}"
                COMMAND ${MCS_EXECUTABLE} ${target_CS_FLAGS} -out:${target_EXE} ${proper_file_list}
                COMMAND ${CMAKE_COMMAND} -E touch ${target_TOUCH}
                MAIN_DEPENDENCY ${ARGN}
                DEPENDS ${ARGN}
                COMMENT "Building ${relative_path}")
        ADD_CUSTOM_TARGET ("${target}" ALL DEPENDS "${target_TOUCH}")

        FOREACH(ref ${${target}_REFS})
                GET_TARGET_PROPERTY(is_target ${ref} TYPE)
                IF(is_target)
                        ADD_DEPENDENCIES(${target} ${ref})
                ENDIF(is_target)
        ENDFOREACH(ref ${${target}_REFS})

        SET(relative_path "")
        SET(proper_file_list "")
ENDMACRO(ADD_CS_EXECUTABLE)

MACRO(INSTALL_GAC target)
        GET_CS_LIBRARY_TARGET_DIR()
        
        INSTALL(CODE "EXECUTE_PROCESS(COMMAND ${GACUTIL_EXECUTABLE} -i ${CS_LIBRARY_TARGET_DIR}/${target}.dll -package 2.0)")
ENDMACRO(INSTALL_GAC target)
